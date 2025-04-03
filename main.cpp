#include <iostream>
#include <vector>
#include <quadmath.h> // Dodano dla __float128
#include <tuple> // Dodano dla std::tuple
using namespace std;

// Struktura przechowująca współczynniki segmentu splajnu
struct SplineSegment {
    __float128 a, b, c, d; // Współczynniki: S(x) = a + b*(x-x_i) + c*(x-x_i)^2 + d*(x-x_i)^3
    __float128 x;          // Początek przedziału
};

class NaturalCubicSpline {
private:
    vector<__float128> x, y, h;
    vector<SplineSegment> segments;

public:
    // Konstruktor przyjmujący wektory punktów (x, y)
    NaturalCubicSpline(const vector<__float128>& x_in, const vector<__float128>& y_in) {
        x = x_in;
        y = y_in;
        int n = x.size();
        h.resize(n - 1);
        
        // Obliczenie kroków h[i] = x[i+1] - x[i]
        for (int i = 0; i < n - 1; i++) {
            h[i] = x[i + 1] - x[i];
        }
        
        // Algorytm de Boor’a dla splajnów naturalnych (warunki brzegowe: c[0] = c[n-1] = 0)
        vector<__float128> alpha(n, 0.0Q), l(n, 0.0Q), mu(n, 0.0Q), z(n, 0.0Q);
        // Warunki początkowe
        l[0] = 1.0Q;
        mu[0] = 0.0Q;
        z[0] = 0.0Q;
        
        // Tworzenie układu równań dla c[1] ... c[n-2]
        for (int i = 1; i < n - 1; i++) {
            alpha[i] = 3.0Q * ((y[i + 1] - y[i]) / h[i] - (y[i] - y[i - 1]) / h[i - 1]);
            l[i] = 2.0Q * (x[i + 1] - x[i - 1]) - h[i - 1] * mu[i - 1];
            mu[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
        }
        // Warunek na końcu
        l[n - 1] = 1.0Q;
        z[n - 1] = 0.0Q;
        
        // Wektory współczynników b i d
        vector<__float128> c(n, 0.0Q), b(n - 1, 0.0Q), d(n - 1, 0.0Q);
        c[n - 1] = 0.0Q;
        
        // Rozwiązywanie układu wstecz (algorytm Thomasa)
        for (int j = n - 2; j >= 0; j--) {
            c[j] = z[j] - mu[j] * c[j + 1];
            b[j] = (y[j + 1] - y[j]) / h[j] - h[j] * (c[j + 1] + 2.0Q * c[j]) / 3.0Q;
            d[j] = (c[j + 1] - c[j]) / (3.0Q * h[j]);
        }
        
        // Utworzenie segmentów splajnu
        segments.resize(n - 1);
        for (int i = 0; i < n - 1; i++) {
            segments[i].a = y[i];      // a = y[i]
            segments[i].b = b[i];
            segments[i].c = c[i];
            segments[i].d = d[i];
            segments[i].x = x[i];
        }
    }
    
    // Funkcja obliczająca wartość splajnu dla zadanego x i zwracająca współczynniki
    std::tuple<__float128, __float128, __float128, __float128, __float128> evaluate(__float128 xi) {
        int n = segments.size();
        int seg = 0;
        // Wyszukanie odpowiedniego segmentu (prosty sposób)
        if (xi <= segments[0].x) {
            seg = 0;
        } else if (xi >= segments[n - 1].x) {
            seg = n - 1;
        } else {
            for (int i = 0; i < n; i++) {
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
};

#include <fstream>
#include <sstream>

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
    inputFile >> n; // Liczba węzłów

    vector<__float128> x(n), y(n);
    for (int i = 0; i < n; i++) {
        double temp;
        inputFile >> temp;
        x[i] = temp;
    }

    for (int i = 0; i < n; i++) {
        double temp;
        inputFile >> temp;
        y[i] = temp;
    }
    if(tryb==1){
        // Tworzenie obiektu NaturalCubicSpline
        NaturalCubicSpline spline(x, y);

        // Test: obliczanie wartości splajnu w przedziale
        for (__float128 xi = x[0]; xi <= x[n - 1]; xi += 0.1Q) {
            char buffer[128];
            char a_buffer[128], b_buffer[128], c_buffer[128], d_buffer[128];
            auto [value, a, b, c, d] = spline.evaluate(xi);
            quadmath_snprintf(buffer, sizeof(buffer), "%.15Qf", value);
            quadmath_snprintf(a_buffer, sizeof(a_buffer), "%.15Qf", a);
            quadmath_snprintf(b_buffer, sizeof(b_buffer), "%.15Qf", b);
            quadmath_snprintf(c_buffer, sizeof(c_buffer), "%.15Qf", c);
            quadmath_snprintf(d_buffer, sizeof(d_buffer), "%.15Qf", d);
            char xi_buffer[128];
            quadmath_snprintf(xi_buffer, sizeof(xi_buffer), "%.15Qf", xi);
            outputFile << "x = " << xi_buffer << ", S(x) = " << buffer
                       << ", a = " << a_buffer << ", b = " << b_buffer
                       << ", c = " << c_buffer << ", d = " << d_buffer << endl;
        }
    }
    inputFile.close();
    outputFile.close();

    return 0;
}
//g++ -o main main.cpp -lquadmath