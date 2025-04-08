#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <quadmath.h>         // Dla __float128 w trybie 1
#include <boost/numeric/interval.hpp> // Dla arytmetyki przedziałowej w trybie 2
#include <tuple>
#include <stdexcept>

// Definicja typu Interval z Boost.Interval dla trybu 2
using Interval = boost::numeric::interval<double>;

// Struktura przechowująca współczynniki segmentu splajnu dla trybu 1 (__float128)
struct SplineSegment {
    __float128 a, b, c, d; // Współczynniki: S(x) = a + b*(x-x_i) + c*(x-x_i)^2 + d*(x-x_i)^3
    double x;              // Początek przedziału (double, bo xi w evaluate jest double)
};

// Struktura przechowująca współczynniki segmentu splajnu dla trybu 2 (Interval)
struct SplineSegmentInterval {
    Interval a, b, c, d; // Współczynniki: S(x) = a + b*(x-x_i) + c*(x-x_i)^2 + d*(x-x_i)^3
    Interval x;          // Początek przedziału jako Interval
};

// Klasa dla interpolacji splajnami naturalnymi w trybie 1 (__float128)
class NaturalCubicSpline {
private:
    std::vector<double> x;
    std::vector<__float128> y, h;
    std::vector<SplineSegment> segments;

public:
    NaturalCubicSpline(const std::vector<double>& x_in, const std::vector<double>& y_in) {
        x = x_in;
        y.resize(x.size());
        for (size_t i = 0; i < x.size(); i++) {
            y[i] = (__float128)y_in[i];
        }
        int n = x.size();

        // Sprawdzenie poprawności węzłów x
        if (n < 2) {
            throw std::invalid_argument("Muszą być co najmniej 2 węzły.");
        }
        for (int i = 0; i < n - 1; i++) {
            if (x[i] >= x[i + 1]) {
                throw std::invalid_argument("Węzły x muszą być posortowane rosnąco i unikalne.");
            }
        }

        // Obliczenie kroków h[i] = x[i+1] - x[i]
        h.resize(n - 1);
        for (int i = 0; i < n - 1; i++) {
            h[i] = (__float128)(x[i + 1] - x[i]);
            if (h[i] <= 0) { // Redundantne po sprawdzeniu powyżej, ale dla pewności
                throw std::runtime_error("h[i] musi być dodatnie.");
            }
        }

        // Algorytm dla splajnów naturalnych
        std::vector<__float128> alpha(n, 0.0), l(n, 0.0), mu(n, 0.0), z(n, 0.0);
        l[0] = 1.0;
        mu[0] = 0.0;
        z[0] = 0.0;

        for (int i = 1; i < n - 1; i++) {
            alpha[i] = 3.0 * ((y[i + 1] - y[i]) / h[i] - (y[i] - y[i - 1]) / h[i - 1]);
            l[i] = 2.0 * (x[i + 1] - x[i - 1]) - h[i - 1] * mu[i - 1];
            mu[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
        }
        l[n - 1] = 1.0;
        z[n - 1] = 0.0;

        std::vector<__float128> c(n, 0.0), b(n - 1, 0.0), d(n - 1, 0.0);
        c[n - 1] = 0.0;

        for (int j = n - 2; j >= 0; j--) {
            c[j] = z[j] - mu[j] * c[j + 1];
            b[j] = (y[j + 1] - y[j]) / h[j] - h[j] * (c[j + 1] + 2.0 * c[j]) / 3.0;
            d[j] = (c[j + 1] - c[j]) / (3.0 * h[j]);
        }

        segments.resize(n - 1);
        for (int i = 0; i < n - 1; i++) {
            segments[i].a = y[i];
            segments[i].b = b[i];
            segments[i].c = c[i];
            segments[i].d = d[i];
            segments[i].x = x[i];
        }
    }

    std::tuple<__float128, __float128, __float128, __float128, __float128> evaluate(double xi) {
        int n = segments.size();
        int seg = 0;
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
        __float128 dx = (__float128)xi - (__float128)segments[seg].x;
        __float128 value = segments[seg].a + segments[seg].b * dx +
                           segments[seg].c * dx * dx +
                           segments[seg].d * dx * dx * dx;
        return {value, segments[seg].a, segments[seg].b, segments[seg].c, segments[seg].d};
    }
};

// Klasa dla interpolacji splajnami naturalnymi w trybie 2 (arytmetyka przedziałowa)
class NaturalCubicSplineInterval {
private:
    std::vector<Interval> x, y, h;
    std::vector<SplineSegmentInterval> segments;

public:
    NaturalCubicSplineInterval(const std::vector<Interval>& x_in, const std::vector<Interval>& y_in) {
        x = x_in;
        y = y_in;
        int n = x.size();

        // Sprawdzenie poprawności węzłów x
        if (n < 2) {
            throw std::invalid_argument("Muszą być co najmniej 2 węzły.");
        }
        for (int i = 0; i < n - 1; i++) {
            if (x[i].upper() >= x[i + 1].lower()) {
                throw std::invalid_argument("Przedziały x muszą być rozłączne i posortowane rosnąco.");
            }
        }

        // Obliczenie kroków h[i] = x[i+1] - x[i]
        h.resize(n - 1);
        for (int i = 0; i < n - 1; i++) {
            h[i] = x[i + 1] - x[i];
            if (h[i].lower() <= 0) {
                throw std::invalid_argument("h[i] musi być dodatnie dla wszystkich wartości w przedziale.");
            }
        }

        // Algorytm dla splajnów naturalnych w arytmetyce przedziałowej
        std::vector<Interval> alpha(n, Interval(0.0)), l(n, Interval(0.0)), mu(n, Interval(0.0)), z(n, Interval(0.0));
        l[0] = Interval(1.0);
        mu[0] = Interval(0.0);
        z[0] = Interval(0.0);

        for (int i = 1; i < n - 1; i++) {
            alpha[i] = Interval(3.0) * ((y[i + 1] - y[i]) / h[i] - (y[i] - y[i - 1]) / h[i - 1]);
            l[i] = Interval(2.0) * (x[i + 1] - x[i - 1]) - h[i - 1] * mu[i - 1];
            mu[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
        }
        l[n - 1] = Interval(1.0);
        z[n - 1] = Interval(0.0);

        std::vector<Interval> c(n, Interval(0.0)), b(n - 1, Interval(0.0)), d(n - 1, Interval(0.0));
        c[n - 1] = Interval(0.0);

        for (int j = n - 2; j >= 0; j--) {
            c[j] = z[j] - mu[j] * c[j + 1];
            b[j] = (y[j + 1] - y[j]) / h[j] - h[j] * (c[j + 1] + Interval(2.0) * c[j]) / Interval(3.0);
            d[j] = (c[j + 1] - c[j]) / (Interval(3.0) * h[j]);
        }

        segments.resize(n - 1);
        for (int i = 0; i < n - 1; i++) {
            segments[i].a = y[i];
            segments[i].b = b[i];
            segments[i].c = c[i];
            segments[i].d = d[i];
            segments[i].x = x[i];
        }
    }

    std::tuple<Interval, Interval, Interval, Interval, Interval> evaluate(double xi) {
        int n = segments.size();
        int seg = 0;
        if (xi <= segments[0].x.lower()) {
            seg = 0;
        } else if (xi >= segments[n - 1].x.upper()) {
            seg = n - 1;
        } else {
            for (int i = 0; i < n; i++) {
                if (xi < segments[i].x.lower()) {
                    seg = i - 1;
                    break;
                }
            }
        }
        Interval dx = Interval(xi) - segments[seg].x;
        Interval value = segments[seg].a + segments[seg].b * dx +
                         segments[seg].c * dx * dx +
                         segments[seg].d * dx * dx * dx;
        return {value, segments[seg].a, segments[seg].b, segments[seg].c, segments[seg].d};
    }
};

int main() {
    std::ifstream inputFile("input.txt");
    std::ofstream outputFile("output.txt");

    if (!inputFile.is_open() || !outputFile.is_open()) {
        std::cerr << "Błąd otwarcia pliku!" << std::endl;
        return 1;
    }

    int tryb;
    inputFile >> tryb; // Wczytanie trybu (1 lub 2)

    int n;
    inputFile >> n; // Liczba węzłów

    if (tryb == 1) {
        // Tryb 1: obliczenia z użyciem __float128
        std::vector<double> x_double(n), y_double(n);
        for (int i = 0; i < n; i++) {
            inputFile >> x_double[i];
        }
        for (int i = 0; i < n; i++) {
            inputFile >> y_double[i];
        }
        try {
            NaturalCubicSpline spline(x_double, y_double);
            for (double xi = x_double[0]; xi <= x_double[n - 1]; xi += 0.1) {
                auto [value, a, b, c, d] = spline.evaluate(xi);
                char buffer[128];
                quadmath_snprintf(buffer, sizeof(buffer), "%.10Qf", value);
                std::string value_str(buffer);
                quadmath_snprintf(buffer, sizeof(buffer), "%.10Qf", a);
                std::string a_str(buffer);
                quadmath_snprintf(buffer, sizeof(buffer), "%.10Qf", b);
                std::string b_str(buffer);
                quadmath_snprintf(buffer, sizeof(buffer), "%.10Qf", c);
                std::string c_str(buffer);
                quadmath_snprintf(buffer, sizeof(buffer), "%.10Qf", d);
                std::string d_str(buffer);

                outputFile << "x = " << xi << ", S(x) = " << value_str
                           << ", a = " << a_str << ", b = " << b_str
                           << ", c = " << c_str << ", d = " << d_str << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Błąd: " << e.what() << std::endl;
            return 1;
        }
    } else if (tryb == 2) {
        // Tryb 2: obliczenia w arytmetyce przedziałowej
        std::vector<double> x_lower(n), x_upper(n), y_lower(n), y_upper(n);
        for (int i = 0; i < n; i++) {
            inputFile >> x_lower[i] >> x_upper[i];
        }
        for (int i = 0; i < n; i++) {
            inputFile >> y_lower[i] >> y_upper[i];
        }
        std::vector<Interval> x(n), y(n);
        for (int i = 0; i < n; i++) {
            x[i] = Interval(x_lower[i], x_upper[i]);
            y[i] = Interval(y_lower[i], y_upper[i]);
        }
        try {
            NaturalCubicSplineInterval spline(x, y);
            double start = x[0].lower();
            double end = x[n - 1].upper();
            for (double xi = start; xi <= end; xi += 0.1) {
                auto [value, a, b, c, d] = spline.evaluate(xi);
                outputFile << "x = " << xi
                           << ", S(x) = [" << value.lower() << ", " << value.upper() << "]"
                           << ", a = [" << a.lower() << ", " << a.upper() << "]"
                           << ", b = [" << b.lower() << ", " << b.upper() << "]"
                           << ", c = [" << c.lower() << ", " << c.upper() << "]"
                           << ", d = [" << d.lower() << ", " << d.upper() << "]" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Błąd: " << e.what() << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Nieprawidłowy tryb!" << std::endl;
        return 1;
    }

    inputFile.close();
    outputFile.close();

    return 0;
}