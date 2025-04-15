#include <iostream>
#include <vector>
#include <quadmath.h>
#include <tuple>
#include <fstream>
#include <sstream>
using namespace std;

struct SplineSegment {
    __float128 a, b, c, d; // Współczynniki lokalne: S(x) = a + b*(x-x_i) + (c/2)*(x-x_i)^2 + d*(x-x_i)^3
    __float128 x;         // Początek przedziału
    // Współczynniki w postaci globalnej:
    // S(x)= a0 + a1*x + a2*x^2 + a3*x^3
    __float128 a0, a1, a2, a3;
};

class NaturalCubicSpline {
private:
    vector<__float128> x, y, h;
    vector<SplineSegment> segments;

public:
    NaturalCubicSpline(const vector<__float128>& x_in, const vector<__float128>& y_in) {
        x = x_in;
        y = y_in;
        int n = x.size();
        h.resize(n - 1);

        // Obliczamy kroki h[i] = x[i+1] - x[i]
        for (int i = 0; i < n - 1; i++) {
            h[i] = x[i + 1] - x[i];
        }

        // Ustawienia do rozwiązania trójdiagonalego układu równań dla naturalnego splajnu
        vector<__float128> alpha(n, 0.0Q), l(n, 0.0Q), mu(n, 0.0Q), z(n, 0.0Q);
        l[0] = 1.0Q;
        mu[0] = 0.0Q;
        z[0] = 0.0Q;

        for (int i = 1; i < n - 1; i++) {
            alpha[i] = 6.0Q * ((y[i + 1] - y[i]) / h[i] - (y[i] - y[i - 1]) / h[i - 1]);
            l[i] = 2.0Q * (x[i + 1] - x[i - 1]) - h[i - 1] * mu[i - 1];
            mu[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
        }
        l[n - 1] = 1.0Q;
        z[n - 1] = 0.0Q;

        // Tablice współczynników – c[i] to wartość przed dalszym podzieleniem przez 2
        vector<__float128> c(n, 0.0Q), b(n - 1, 0.0Q), d(n - 1, 0.0Q);
        c[n - 1] = 0.0Q;

        for (int j = n - 2; j >= 0; j--) {
            c[j] = z[j] - mu[j] * c[j + 1];
            b[j] = (y[j + 1] - y[j]) / h[j] - h[j] * (c[j + 1] + 2.0Q * c[j]) / 6.0Q;
            d[j] = (c[j + 1] - c[j]) / (6.0Q * h[j]);
        }

        // Wypełniamy segmenty splajnów
        // Przyjmujemy postać lokalną:
        // S_i(x) = y[i] + b[i]*(x - x[i]) + (c[i]/2)*(x - x[i])^2 + d[i]*(x - x[i])^3.
        segments.resize(n - 1);
        for (int i = 0; i < n - 1; i++) {
            segments[i].a = y[i];
            segments[i].b = b[i];
            segments[i].c = c[i]; // przechowujemy c[i] bez dzielenia przez 2
            segments[i].d = d[i];
            segments[i].x = x[i];

            // Przekształcenie do postaci globalnej:
            // S(x)= a0 + a1*x + a2*x^2 + a3*x^3, gdzie:
            // a0 = y[i] - b[i]*x[i] + (c[i]/2)*x[i]^2 - d[i]*x[i]^3,
            // a1 = b[i] - c[i]*x[i] + 3*d[i]*x[i]^2,
            // a2 = (c[i]/2) - 3*d[i]*x[i],
            // a3 = d[i].
            __float128 xi = x[i];
            segments[i].a0 = segments[i].a - segments[i].b * xi + (segments[i].c * xi * xi) / 2.0Q - segments[i].d * xi * xi * xi;
            segments[i].a1 = segments[i].b - segments[i].c * xi + 3.0Q * segments[i].d * xi * xi;
            segments[i].a2 = (segments[i].c) / 2.0Q - 3.0Q * segments[i].d * xi;
            segments[i].a3 = segments[i].d;
        }
    }

    // Obliczenie wartości splajnu dla danej xi (przy użyciu postaci lokalnej)
    tuple<__float128, __float128, __float128, __float128, __float128> evaluate(__float128 xi) {
        int n = segments.size();
        if (n == 0) {
            return {0.0Q, 0.0Q, 0.0Q, 0.0Q, 0.0Q};
        }
        int seg = 0;
        if (xi < x[0])
            seg = 0;
        else if (xi >= x[x.size() - 1])
            seg = x.size() - 2; // ostatni segment
        else {
            for (int i = 0; i < x.size() - 1; i++) {
                if (xi >= x[i] && xi < x[i+1]) {
                    seg = i;
                    break;
                }
            }
        }
        __float128 dx = xi - segments[seg].x;
        __float128 value = segments[seg].a + segments[seg].b * dx +
                           (segments[seg].c / 2.0Q) * dx * dx +
                           segments[seg].d * dx * dx * dx;
        return {value, segments[seg].a, segments[seg].b, (segments[seg].c / 2.0Q), segments[seg].d};
    }

    // Funkcja wypisująca współczynniki globalne w układzie a[0-3, 0-(n-1)],
    // czyli najpierw wypisujemy kolejno a0 dla wszystkich segmentów, potem a1 itd.
    void printCoefficients(ofstream& outputFile) {
        const int numCoeff = 4; // a0, a1, a2, a3
        int numSegments = segments.size();
        for (int coeff = 0; coeff < numCoeff; coeff++) {
            for (int seg = 0; seg < numSegments; seg++) {
                char buffer[128];
                __float128 value;
                if (coeff == 0)       value = segments[seg].a0;
                else if (coeff == 1)  value = segments[seg].a1;
                else if (coeff == 2)  value = segments[seg].a2;
                else /* coeff==3 */   value = segments[seg].a3;
                quadmath_snprintf(buffer, sizeof(buffer), "%.9Qf", value);
                outputFile << "a[" << coeff << "," << seg << "] = " << buffer << " ";
            }
            outputFile << "\n";
        }
    }
};

int main() {
    ifstream inputFile("input.txt");
    ofstream outputFile("output.txt");

    if (!inputFile.is_open() || !outputFile.is_open()) {
        cerr << "Blad otwarcia pliku!" << endl;
        return 1;
    }

    int tryb;
    inputFile >> tryb;

    int n;
    inputFile >> n;

    vector<__float128> x(n), y(n);
    for (int i = 0; i < n; i++) {
        char buffer[128];
        inputFile >> buffer;
        x[i] = strtoflt128(buffer, NULL);
    }
    for (int i = 0; i < n; i++) {
        char buffer[128];
        inputFile >> buffer;
        y[i] = strtoflt128(buffer, NULL);
    }

    __float128 xx;
    {
        char buffer[128];
        inputFile >> buffer;
        xx = strtoflt128(buffer, NULL);
    }

    if (tryb == 1) {
        NaturalCubicSpline spline(x, y);
        spline.printCoefficients(outputFile);

        char buffer[128];
        auto [value, a, b, c, d] = spline.evaluate(xx);
        quadmath_snprintf(buffer, sizeof(buffer), "%.15Qf", value);
        char xxBuffer[128];
        quadmath_snprintf(xxBuffer, sizeof(xxBuffer), "%.15Qf", xx);
        outputFile << "S(" << xxBuffer << ") = " << buffer << "\n";
    }

    inputFile.close();
    outputFile.close();
    return 0;
}
// Kompilacja: g++ -o main main.cpp -lquadmath -lm