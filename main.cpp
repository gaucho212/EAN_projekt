#include <iostream>
#include <vector>
using namespace std;

// Struktura przechowująca współczynniki segmentu splajnu
struct SplineSegment {
    double a, b, c, d; // Współczynniki: S(x) = a + b*(x-x_i) + c*(x-x_i)^2 + d*(x-x_i)^3
    double x;         // Początek przedziału
};

class NaturalCubicSpline {
private:
    vector<double> x, y, h;
    vector<SplineSegment> segments;

public:
    // Konstruktor przyjmujący wektory punktów (x, y)
    NaturalCubicSpline(const vector<double>& x_in, const vector<double>& y_in) {
        x = x_in;
        y = y_in;
        int n = x.size();
        h.resize(n - 1);
        
        // Obliczenie kroków h[i] = x[i+1] - x[i]
        for (int i = 0; i < n - 1; i++) {
            h[i] = x[i + 1] - x[i];
        }
        
        // Algorytm de Boor’a dla splajnów naturalnych (warunki brzegowe: c[0] = c[n-1] = 0)
        vector<double> alpha(n, 0.0), l(n, 0.0), mu(n, 0.0), z(n, 0.0);
        // Warunki początkowe
        l[0] = 1.0;
        mu[0] = 0.0;
        z[0] = 0.0;
        
        // Tworzenie układu równań dla c[1] ... c[n-2]
        for (int i = 1; i < n - 1; i++) {
            alpha[i] = 3.0 * ((y[i + 1] - y[i]) / h[i] - (y[i] - y[i - 1]) / h[i - 1]);
            l[i] = 2.0 * (x[i + 1] - x[i - 1]) - h[i - 1] * mu[i - 1];
            mu[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
        }
        // Warunek na końcu
        l[n - 1] = 1.0;
        z[n - 1] = 0.0;
        
        // Wektory współczynników b i d
        vector<double> c(n, 0.0), b(n - 1, 0.0), d(n - 1, 0.0);
        c[n - 1] = 0.0;
        
        // Rozwiązywanie układu wstecz (algorytm Thomasa)
        for (int j = n - 2; j >= 0; j--) {
            c[j] = z[j] - mu[j] * c[j + 1];
            b[j] = (y[j + 1] - y[j]) / h[j] - h[j] * (c[j + 1] + 2.0 * c[j]) / 3.0;
            d[j] = (c[j + 1] - c[j]) / (3.0 * h[j]);
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
    
    // Funkcja obliczająca wartość splajnu dla zadanego x
    double evaluate(double xi) {
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
        double dx = xi - segments[seg].x;
        return segments[seg].a + segments[seg].b * dx +
               segments[seg].c * dx * dx +
               segments[seg].d * dx * dx * dx;
    }
};

int main() {
    int n;
    cout << "Podaj liczbe wezlow: ";
    cin >> n;

    vector<double> x(n), y(n);
    for (int i = 0; i < n; i++) {
        cout << "Podaj punkty x[",i,"]:";
        cin >> x[i];
    }

    for (int i = 0; i < n; i++) {
        cout << "Podaj punkty x[",y,"]:";
        cin >> y[i];
    }
    
    // Tworzenie obiektu NaturalCubicSpline
    NaturalCubicSpline spline(x, y);
    
    // Test: obliczanie wartości splajnu w przedziale
    for (double xi = x[0]; xi <= x[n - 1]; xi += 0.1) {
        cout << "x = " << xi << ", S(x) = " << spline.evaluate(xi) << endl;
    }
    
    return 0;
}
// Kompilacja: g++ -o spline main.cpp -std=c++11