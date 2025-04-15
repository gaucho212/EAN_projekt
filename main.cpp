#include <iostream>
#include <vector>
#include <quadmath.h>
#include <tuple>
#include <fstream>
#include <sstream>
#include <cstdlib>
using namespace std;

// ====================
// Dla trybu 1 (float128)
// ====================
struct SplineSegment {
    __float128 a, b, c, d; // współczynniki lokalne: S(x) = a + b*(x-x_i) + (c/2)*(x-x_i)^2 + d*(x-x_i)^3
    __float128 x;         // początek przedziału
    // współczynniki globalne (postać S(x)= a0 + a1*x + a2*x^2 + a3*x^3)
    __float128 a0, a1, a2, a3;
};

class NaturalCubicSpline {
private:
    vector<__float128> x, y, h;
    vector<SplineSegment> segments;
public:
    NaturalCubicSpline(const vector<__float128>& x_in, const vector<__float128>& y_in) {
        x = x_in; y = y_in;
        int n = x.size();
        h.resize(n - 1);
        for (int i = 0; i < n - 1; i++) {
            h[i] = x[i + 1] - x[i];
        }
        // Układ równań dla naturalnego splajnu
        vector<__float128> alpha(n, 0.0Q), l(n, 0.0Q), mu(n, 0.0Q), z(n, 0.0Q);
        l[0] = 1.0Q; mu[0] = 0.0Q; z[0] = 0.0Q;
        for (int i = 1; i < n - 1; i++) {
            alpha[i] = 6.0Q * ((y[i + 1] - y[i]) / h[i] - (y[i] - y[i - 1]) / h[i - 1]);
            l[i] = 2.0Q * (x[i + 1] - x[i - 1]) - h[i - 1] * mu[i - 1];
            mu[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
        }
        l[n - 1] = 1.0Q; z[n - 1] = 0.0Q;
        vector<__float128> c(n, 0.0Q), b(n - 1, 0.0Q), d(n - 1, 0.0Q);
        c[n - 1] = 0.0Q;
        for (int j = n - 2; j >= 0; j--) {
            c[j] = z[j] - mu[j] * c[j + 1];
            b[j] = (y[j + 1] - y[j]) / h[j] - h[j] * (c[j + 1] + 2.0Q * c[j]) / 6.0Q;
            d[j] = (c[j + 1] - c[j]) / (6.0Q * h[j]);
        }
        // Wypełniamy segmenty
        segments.resize(n - 1);
        for (int i = 0; i < n - 1; i++) {
            segments[i].a = y[i];
            segments[i].b = b[i];
            segments[i].c = c[i]; // zachowujemy oryginalne c[i]
            segments[i].d = d[i];
            segments[i].x = x[i];
            __float128 xi = x[i];
            segments[i].a0 = segments[i].a - segments[i].b * xi + (segments[i].c * xi * xi) / 2.0Q - segments[i].d * xi * xi * xi;
            segments[i].a1 = segments[i].b - segments[i].c * xi + 3.0Q * segments[i].d * xi * xi;
            segments[i].a2 = (segments[i].c) / 2.0Q - 3.0Q * segments[i].d * xi;
            segments[i].a3 = segments[i].d;
        }
    }
    
    // Obliczenie S(xi) przy użyciu postaci lokalnej
    tuple<__float128, __float128, __float128, __float128, __float128> evaluate(__float128 xi) {
        int n = segments.size();
        if (n == 0) return {0.0Q,0.0Q,0.0Q,0.0Q,0.0Q};
        int seg = 0;
        if (xi < x[0])
            seg = 0;
        else if (xi >= x[x.size()-1])
            seg = x.size()-2;
        else {
            for (int i = 0; i < x.size()-1; i++) {
                if (xi >= x[i] && xi < x[i+1]) { seg = i; break; }
            }
        }
        __float128 dx = xi - segments[seg].x;
        __float128 value = segments[seg].a + segments[seg].b * dx +
                           (segments[seg].c / 2.0Q) * dx * dx +
                           segments[seg].d * dx * dx * dx;
        return {value, segments[seg].a, segments[seg].b, (segments[seg].c/2.0Q), segments[seg].d};
    }
    
    // Wypisanie współczynników globalnych (macierz a[0..3, 0..(n-2)])
    void printCoefficients(ofstream& outputFile) {
        const int numCoeff = 4;
        int numSegments = segments.size();
        for (int coeff = 0; coeff < numCoeff; coeff++) {
            for (int seg = 0; seg < numSegments; seg++) {
                char buffer[128];
                __float128 value;
                if (coeff==0)       value = segments[seg].a0;
                else if (coeff==1)  value = segments[seg].a1;
                else if (coeff==2)  value = segments[seg].a2;
                else                value = segments[seg].a3;
                quadmath_snprintf(buffer, sizeof(buffer), "%.9Qf", value);
                outputFile << "a[" << coeff << "," << seg << "] = " << buffer << " ";
            }
            outputFile << "\n";
        }
    }
};

// ====================
// Dla trybu 2 (arytmetyka przedziałowa)
// ====================

struct Interval {
    __float128 lo, hi;
};

// Konstruktor przedziału ze skalara (obustronnie taki sam)
Interval I(__float128 v) {
    Interval r; r.lo = v; r.hi = v; return r;
}

Interval add(const Interval &a, const Interval &b) {
    Interval r;
    r.lo = a.lo + b.lo;
    r.hi = a.hi + b.hi;
    return r;
}

Interval subInt(const Interval &a, const Interval &b) {
    // odejmowanie: [a.lo - b.hi, a.hi - b.lo]
    Interval r;
    r.lo = a.lo - b.hi;
    r.hi = a.hi - b.lo;
    return r;
}

Interval mul(const Interval &a, const Interval &b) {
    __float128 p1 = a.lo * b.lo;
    __float128 p2 = a.lo * b.hi;
    __float128 p3 = a.hi * b.lo;
    __float128 p4 = a.hi * b.hi;
    Interval r;
    r.lo = p1;
    if (p2 < r.lo) r.lo = p2;
    if (p3 < r.lo) r.lo = p3;
    if (p4 < r.lo) r.lo = p4;
    r.hi = p1;
    if (p2 > r.hi) r.hi = p2;
    if (p3 > r.hi) r.hi = p3;
    if (p4 > r.hi) r.hi = p4;
    return r;
}

Interval divInt(const Interval &a, const Interval &b) {
    // Zakładamy, że przedział b nie zawiera 0.
    __float128 p1 = a.lo / b.lo;
    __float128 p2 = a.lo / b.hi;
    __float128 p3 = a.hi / b.lo;
    __float128 p4 = a.hi / b.hi;
    Interval r;
    r.lo = p1;
    if (p2 < r.lo) r.lo = p2;
    if (p3 < r.lo) r.lo = p3;
    if (p4 < r.lo) r.lo = p4;
    r.hi = p1;
    if (p2 > r.hi) r.hi = p2;
    if (p3 > r.hi) r.hi = p3;
    if (p4 > r.hi) r.hi = p4;
    return r;
}

Interval square(const Interval &a) {
    return mul(a, a);
}

Interval cube(const Interval &a) {
    return mul(a, square(a));
}

// Funkcja pomocnicza do wypisywania przedziału jako string "[lo, hi]"
string toString(const Interval &a) {
    char bufLo[128], bufHi[128];
    quadmath_snprintf(bufLo, sizeof(bufLo), "%.9Qf", a.lo);
    quadmath_snprintf(bufHi, sizeof(bufHi), "%.9Qf", a.hi);
    string s = "[";
    s += bufLo; s += ", "; s += bufHi; s += "]";
    return s;
}

struct IntervalSplineSegment {
    Interval a, b, c, d; // współczynniki lokalne
    Interval x;          // początek przedziału
    // współczynniki globalne
    Interval a0, a1, a2, a3;
};

class NaturalCubicSplineInterval {
private:
    vector<Interval> x, y, h;
    vector<IntervalSplineSegment> segments;
public:
    // Konstruktor przyjmujący wektory przedziałów dla x i y
    NaturalCubicSplineInterval(const vector<Interval>& x_in, const vector<Interval>& y_in) {
        x = x_in; y = y_in;
        int n = x.size();
        h.resize(n - 1);
        for (int i = 0; i < n - 1; i++) {
            h[i] = subInt(x[i+1], x[i]);
        }
        // Przygotowanie układu równań – wszystkie zmienne jako przedziały
        vector<Interval> alpha(n, I(0.0Q)), l(n, I(0.0Q)), mu(n, I(0.0Q)), z(n, I(0.0Q));
        l[0] = I(1.0Q); mu[0] = I(0.0Q); z[0] = I(0.0Q);
        for (int i = 1; i < n - 1; i++) {
            // alpha[i] = 6 * [ (y[i+1]-y[i])/h[i] - (y[i]-y[i-1])/h[i-1] ]
            Interval diff1 = divInt( subInt(y[i+1], y[i]), h[i] );
            Interval diff2 = divInt( subInt(y[i], y[i-1]), h[i-1] );
            alpha[i] = mul( I(6.0Q), subInt(diff1, diff2) );
            l[i] = subInt( mul(I(2.0Q), subInt(x[i+1], x[i-1]) ), mul( h[i-1], mu[i-1] ) );
            mu[i] = divInt( h[i], l[i] );
            z[i] = divInt( subInt(alpha[i], mul( h[i-1], z[i-1] ) ), l[i] );
        }
        l[n - 1] = I(1.0Q); z[n - 1] = I(0.0Q);
        vector<Interval> c(n, I(0.0Q)), b(n - 1, I(0.0Q)), d(n - 1, I(0.0Q));
        c[n - 1] = I(0.0Q);
        for (int j = n - 2; j >= 0; j--) {
            c[j] = subInt(z[j], mul( mu[j], c[j+1] ) );
            Interval term = add( c[j+1], mul( I(2.0Q), c[j] ) );
            b[j] = subInt( divInt( subInt(y[j+1], y[j]), h[j] ),
                           divInt( mul( h[j], term ), I(6.0Q) ) );
            d[j] = divInt( subInt(c[j+1], c[j]), mul( I(6.0Q), h[j] ) );
        }
        // Wypełnienie segmentów – postać lokalna:
        // S_i(x) = y[i] + b[i]*(x-x[i]) + (c[i]/2)*(x-x[i])^2 + d[i]*(x-x[i])^3.
        segments.resize(n - 1);
        for (int i = 0; i < n - 1; i++) {
            segments[i].a = y[i];
            segments[i].b = b[i];
            segments[i].c = c[i]; // przechowujemy c[i] bez dzielenia przez 2
            segments[i].d = d[i];
            segments[i].x = x[i];
            // Przekształcenie do postaci globalnej:
            // a₀ = a - b*x + (c*x²)/2 - d*x³
            Interval temp1 = subInt( segments[i].a, mul( segments[i].b, x[i] ) );
            Interval temp2 = divInt( mul( segments[i].c, square(x[i]) ), I(2.0Q) );
            Interval temp3 = mul( segments[i].d, cube(x[i]) );
            segments[i].a0 = subInt( add( temp1, temp2 ), temp3 );
            // a₁ = b - c*x + 3*d*x²
            segments[i].a1 = add( subInt( segments[i].b, mul( segments[i].c, x[i] ) ),
                                  mul( I(3.0Q), mul( segments[i].d, square(x[i]) ) ) );
            // a₂ = (c/2) - 3*d*x
            segments[i].a2 = subInt( divInt( segments[i].c, I(2.0Q) ),
                                    mul( I(3.0Q), mul( segments[i].d, x[i] ) ) );
            segments[i].a3 = segments[i].d;
        }
    }
    
    // Obliczenie S(xi) (postać lokalna) – xi jest przedziałem
    tuple<Interval, Interval, Interval, Interval, Interval> evaluate(const Interval &xi) {
        int n = segments.size();
        if(n == 0) return {I(0.0Q), I(0.0Q), I(0.0Q), I(0.0Q), I(0.0Q)};
        int seg = 0;
        // Wybór segmentu analogicznie do trybu 1 – operujemy na dolnych i górnych granicach
        if(xi.lo < x[0].lo)
            seg = 0;
        else if(xi.hi >= x[x.size()-1].hi)
            seg = x.size()-2;
        else {
            for (int i = 0; i < x.size()-1; i++) {
                if(xi.lo >= x[i].lo && xi.hi < x[i+1].hi) { seg = i; break; }
            }
        }
        Interval dx = subInt(xi, segments[seg].x);
        Interval term1 = segments[seg].a;
        Interval term2 = mul(segments[seg].b, dx);
        Interval term3 = mul( divInt( segments[seg].c, I(2.0Q) ), square(dx) );
        Interval term4 = mul( segments[seg].d, cube(dx) );
        Interval value = add( add(term1, term2), add(term3, term4) );
        return {value, segments[seg].a, segments[seg].b, divInt(segments[seg].c, I(2.0Q)), segments[seg].d};
    }
    
    // Wypisanie współczynników globalnych – przedziały wypisywane w formacie "[lo, hi]"
    void printCoefficients(ofstream &outputFile) {
        const int numCoeff = 4; 
        int numSegments = segments.size();
        for (int coeff = 0; coeff < numCoeff; coeff++) {
            for (int seg = 0; seg < numSegments; seg++) {
                Interval val;
                if(coeff==0)       val = segments[seg].a0;
                else if(coeff==1)  val = segments[seg].a1;
                else if(coeff==2)  val = segments[seg].a2;
                else               val = segments[seg].a3;
                outputFile << "a[" << coeff << "," << seg << "] = " << toString(val) << " ";
            }
            outputFile << "\n";
        }
    }
};


// ====================
// main
// ====================
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
    
    if (tryb == 1) {
        // Tryb klasyczny – dla każdego n wczytujemy jedną wartość (w formacie tekstowym, konwertowaną przez strtoflt128)
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
        NaturalCubicSpline spline(x,y);
        spline.printCoefficients(outputFile);
        char buffer[128];
        auto [value, a, b, c, d] = spline.evaluate(xx);
        quadmath_snprintf(buffer, sizeof(buffer), "%.15Qf", value);
        char xxBuffer[128];
        quadmath_snprintf(xxBuffer, sizeof(xxBuffer), "%.15Qf", xx);
        outputFile << "S(" << xxBuffer << ") = " << buffer << "\n";
    } else if (tryb == 2) {
        // Tryb przedziałowy – dla każdego n wczytujemy dwie wartości: dolną i górną granicę
        vector<Interval> x(n), y(n);
        for (int i = 0; i < n; i++) {
            char bufLo[128], bufHi[128];
            inputFile >> bufLo >> bufHi;
            x[i].lo = strtoflt128(bufLo, NULL);
            x[i].hi = strtoflt128(bufHi, NULL);
        }
        for (int i = 0; i < n; i++) {
            char bufLo[128], bufHi[128];
            inputFile >> bufLo >> bufHi;
            y[i].lo = strtoflt128(bufLo, NULL);
            y[i].hi = strtoflt128(bufHi, NULL);
        }
        Interval xx;
        {
            char bufLo[128], bufHi[128];
            inputFile >> bufLo >> bufHi;
            xx.lo = strtoflt128(bufLo, NULL);
            xx.hi = strtoflt128(bufHi, NULL);
        }
        NaturalCubicSplineInterval spline(x,y);
        spline.printCoefficients(outputFile);
        auto [value, a, b, c, d] = spline.evaluate(xx);
        outputFile << "S(" << toString(xx) << ") = " << toString(value) << "\n";
    }
    
    inputFile.close();
    outputFile.close();
    return 0;
}
// g++ -o main main.cpp -lquadmath -lm
