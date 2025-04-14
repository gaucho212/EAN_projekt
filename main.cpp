#include <iostream>
#include <vector>
#include <quadmath.h>
#include <tuple>
#include <fstream>
#include <sstream>
using namespace std;

struct SplineSegment {
    __float128 a, b, c, d; // Współczynniki: S(x) = a + b*(x-x_i) + c*(x-x_i)^2 + d*(x-x_i)^3
    __float128 x;          // Początek przedziału
    __float128 a0, a1, a2, a3; // Współczynniki w formie standardowej
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

        for (int i = 0; i < n - 1; i++) {
            h[i] = x[i + 1] - x[i];
        }

        vector<__float128> alpha(n, 0.0Q), l(n, 0.0Q), mu(n, 0.0Q), z(n, 0.0Q);
        l[0] = 1.0Q;
        mu[0] = 0.0Q;
        z[0] = 0.0Q;

        for (int i = 1; i < n - 1; i++) {
            alpha[i] = 3.0Q * ((y[i + 1] - y[i]) / h[i] - (y[i] - y[i - 1]) / h[i - 1]);
            l[i] = 2.0Q * (x[i + 1] - x[i - 1]) - h[i - 1] * mu[i - 1];
            mu[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
        }
        l[n - 1] = 1.0Q;
        z[n - 1] = 0.0Q;

        vector<__float128> c(n, 0.0Q), b(n - 1, 0.0Q), d(n - 1, 0.0Q);
        c[n - 1] = 0.0Q;

        for (int j = n - 2; j >= 0; j--) {
            c[j] = z[j] - mu[j] * c[j + 1];
            b[j] = (y[j + 1] - y[j]) / h[j] - h[j] * (c[j + 1] + 2.0Q * c[j]) / 6.0Q;
            d[j] = (c[j + 1] - c[j]) / (6.0Q * h[j]); // Poprawiona formuła
        }

        segments.resize(n - 1);
        for (int i = 0; i < n - 1; i++) {
            segments[i].a = y[i];
            segments[i].b = b[i];
            segments[i].c = c[i] / 2.0Q; // Poprawiona definicja c
            segments[i].d = d[i];
            segments[i].x = x[i];
            // Obliczenie współczynników standardowych
            __float128 xi = x[i];
            segments[i].a0 = segments[i].a - segments[i].b * xi + segments[i].c * xi * xi - segments[i].d * xi * xi * xi;
            segments[i].a1 = segments[i].b - 2.0Q * segments[i].c * xi + 3.0Q * segments[i].d * xi * xi;
            segments[i].a2 = segments[i].c - 3.0Q * segments[i].d * xi;
            segments[i].a3 = segments[i].d;
        }
    }

    tuple<__float128, __float128, __float128, __float128, __float128> evaluate(__float128 xi) {
        int n = segments.size();
        int seg = 0;
        if (n == 0) return {y[0], 0.0Q, 0.0Q, 0.0Q, 0.0Q};
        if (xi <= segments[0].x) seg = 0;
        else if (xi >= segments[n - 1].x) seg = n - 1;
        else {
            for (int i = 1; i < n; i++) {
                if (xi < segments[i].x) {
                    seg = i - 1;
                    break;
                }
            }
        }
        __float128 dx = xi - segments[seg].x;
        __float128 value = segments[seg].a + segments[seg].b * dx +
                           segments[seg].c * dx * dx +
                           segments[seg].d * dx * dx * dx;
        return {value, segments[seg].a, segments[seg].b, segments[seg].c, segments[seg].d};
    }

    void printCoefficients(ofstream& outputFile) {
        char buffer[128];
        for (int i = 0; i < segments.size(); i++) {
            quadmath_snprintf(buffer, sizeof(buffer), "%.15Qf", segments[i].a0);
            outputFile << "a[" << i << ",0] = " << buffer << " ";
            quadmath_snprintf(buffer, sizeof(buffer), "%.15Qf", segments[i].a1);
            outputFile << "a[" << i << ",1] = " << buffer << " ";
            quadmath_snprintf(buffer, sizeof(buffer), "%.15Qf", segments[i].a2);
            outputFile << "a[" << i << ",2] = " << buffer << " ";
            quadmath_snprintf(buffer, sizeof(buffer), "%.15Qf", segments[i].a3);
            outputFile << "a[" << i << ",3] = " << buffer << "\n";
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