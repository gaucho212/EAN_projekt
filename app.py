import tkinter as tk
from tkinter import messagebox, scrolledtext
import subprocess


def calculate():
    # Pobierz wartości z interfejsu
    mode = mode_var.get()
    nodes_str = nodes_entry.get()
    x_str = x_entry.get()
    y_str = y_entry.get()
    xx_str = xx_entry.get()  # Pobierz wartość xx z nowego pola

    # Sprawdź, czy liczba węzłów jest poprawna
    try:
        nodes = int(nodes_str)
        if nodes <= 0:
            raise ValueError
    except ValueError:
        tk.messagebox.showerror(
            "Błąd", "Liczba węzłów musi być dodatnią liczbą całkowitą."
        )
        return

    # Podziel wartości x i y na listy
    x_values = x_str.split()
    y_values = y_str.split()

    # Sprawdź, czy liczba wartości zgadza się z oczekiwaniami w danym trybie
    if mode == 1:
        # Tryb 1: arytmetyka zmiennoprzecinkowa, pojedyncze wartości
        if len(x_values) != nodes or len(y_values) != nodes:
            tk.messagebox.showerror(
                "Błąd",
                "W trybie 1 liczba wartości x i y musi zgadzać się z liczbą węzłów.",
            )
            return
    elif mode == 2:
        # Tryb 2: arytmetyka przedziałowa, pary wartości (dolna i górna granica)
        if len(x_values) != 2 * nodes or len(y_values) != 2 * nodes:
            tk.messagebox.showerror(
                "Błąd",
                "W trybie 2 należy podać po dwie wartości (dolna i górna granica) dla każdego x i y.",
            )
            return
    else:
        # Tryb 3 lub inne nieobsługiwane tryby
        tk.messagebox.showerror("Błąd", "Wybrano nieobsługiwany tryb.")
        return

    # Konwertuj wartości na liczby zmiennoprzecinkowe
    try:
        x_floats = [float(x) for x in x_values]
        y_floats = [float(y) for y in y_values]
        xx = float(xx_str)  # Konwertuj xx na liczbę zmiennoprzecinkową
    except ValueError:
        tk.messagebox.showerror("Błąd", "Wszystkie wartości muszą być liczbami.")
        return

    # Zapisz dane do pliku input.txt
    with open("input.txt", "w") as f:
        f.write(str(mode) + "\n")
        f.write(str(nodes) + "\n")
        f.write(" ".join(map(str, x_floats)) + "\n")
        f.write(" ".join(map(str, y_floats)) + "\n")
        f.write(str(xx) + "\n")  # Zapisz xx jako ostatnią linię

    # Uruchom program C++
    try:
        subprocess.run(["./main"], check=True)
    except subprocess.CalledProcessError:
        tk.messagebox.showerror("Błąd", "Obliczenia nie powiodły się.")
        return

    # Odczytaj wynik z pliku output.txt
    try:
        with open("output.txt", "r") as f:
            result = f.read().strip()
        result_text.delete(1.0, tk.END)  # Wyczyść poprzedni wynik
        result_text.insert(tk.END, "Wynik:\n" + result)
    except FileNotFoundError:
        tk.messagebox.showerror("Błąd", "Nie znaleziono pliku wyjściowego.")
    except Exception as e:
        tk.messagebox.showerror("Błąd", str(e))


def show_info():
    # Tworzenie nowego okna informacyjnego
    info_window = tk.Toplevel(root)
    info_window.title("Informacje o aplikacji")
    info_window.geometry("600x400")
    info_window.configure(bg="#f0f0f0")

    # Pole tekstowe z informacjami
    info_text = tk.scrolledtext.ScrolledText(
        info_window, width=70, height=20, font=("Arial", 10), bg="#ffffff"
    )
    info_text.pack(padx=10, pady=10, fill="both", expand=True)

    # Treść informacji
    info_content = """
    Aplikacja oblicza wartości i współczynniki naturalnej funkcji splajnu stopnia trzeciego.

    Instrukcje dotyczące wpisywania danych:

    - Tryb 1 (zmiennoprzecinkowy):
      W polu "Wartości x" wpisz wartości x oddzielone spacjami. Liczba wartości musi odpowiadać liczbie węzłów.
      W polu "Wartości y" wpisz wartości y oddzielone spacjami. Liczba wartości musi odpowiadać liczbie węzłów.
      W polu "Punkt xx" wpisz pojedynczą wartość liczbową.

    - Tryb 2 (przedziałowy):
      W polu "Wartości x" wpisz pary wartości (dolna i górna granica) dla każdego x, oddzielone spacjami.
      W polu "Wartości y" wpisz pary wartości (dolna i górna granica) dla każdego y, oddzielone spacjami.
      Liczba wartości musi być dwukrotnością liczby węzłów.
      W polu "Punkt xx" wpisz pojedynczą wartość liczbową (dla uproszczenia).
    """
    info_text.insert(tk.END, info_content)
    info_text.config(state=tk.DISABLED)  # Zablokuj edycję tekstu


# Utwórz główne okno
root = tk.Tk()
root.title("Aplikacja Obliczeniowa")
root.geometry("800x600")  # Ustaw rozmiar okna
root.configure(bg="#f0f0f0")  # Jasne tło

# Styl dla etykiet
label_style = {"font": ("Arial", 12, "bold"), "bg": "#f0f0f0"}

# Ramka dla wyboru trybu
mode_frame = tk.Frame(root, bg="#e0e0e0", bd=2, relief="groove")
mode_frame.pack(pady=10, padx=10, fill="x")

mode_label = tk.Label(mode_frame, text="Wybierz tryb:", **label_style)
mode_label.pack(pady=5)

mode_var = tk.IntVar(value=1)  # Domyślnie tryb 1
tk.Radiobutton(
    mode_frame,
    text="Tryb 1 (zmiennoprzecinkowy)",
    variable=mode_var,
    value=1,
    bg="#e0e0e0",
    font=("Arial", 10),
).pack(anchor="w", padx=10)
tk.Radiobutton(
    mode_frame,
    text="Tryb 2 (przedziałowy)",
    variable=mode_var,
    value=2,
    bg="#e0e0e0",
    font=("Arial", 10),
).pack(anchor="w", padx=10)

# Ramka dla danych wejściowych
input_frame = tk.Frame(root, bg="#e0e0e0", bd=2, relief="groove")
input_frame.pack(pady=10, padx=10, fill="x")

# Liczba węzłów
nodes_label = tk.Label(input_frame, text="Liczba węzłów:", **label_style)
nodes_label.grid(row=0, column=0, padx=5, pady=5, sticky="w")
nodes_entry = tk.Entry(input_frame, width=10, font=("Arial", 10))
nodes_entry.grid(row=0, column=1, padx=5, pady=5)

# Wartości x
x_label = tk.Label(input_frame, text="Wartości x (oddzielone spacjami):", **label_style)
x_label.grid(row=1, column=0, padx=5, pady=5, sticky="w")
x_entry = tk.Entry(input_frame, width=50, font=("Arial", 10))
x_entry.grid(row=1, column=1, padx=5, pady=5)

# Wartości y
y_label = tk.Label(input_frame, text="Wartości y (oddzielone spacjami):", **label_style)
y_label.grid(row=2, column=0, padx=5, pady=5, sticky="w")
y_entry = tk.Entry(input_frame, width=50, font=("Arial", 10))
y_entry.grid(row=2, column=1, padx=5, pady=5)

# Nowe pole dla xx
xx_label = tk.Label(input_frame, text="Punkt xx:", **label_style)
xx_label.grid(row=3, column=0, padx=5, pady=5, sticky="w")
xx_entry = tk.Entry(input_frame, width=10, font=("Arial", 10))
xx_entry.grid(row=3, column=1, padx=5, pady=5)

# Przycisk obliczeń
calculate_button = tk.Button(
    root,
    text="Oblicz",
    command=calculate,
    font=("Arial", 12, "bold"),
    bg="#4CAF50",
    fg="white",
    relief="raised",
)
calculate_button.pack(pady=10)

# Przycisk informacyjny
info_button = tk.Button(
    root,
    text="Informacje",
    command=show_info,
    font=("Arial", 12, "bold"),
    bg="#2196F3",
    fg="white",
    relief="raised",
)
info_button.pack(pady=10)

# Ramka dla wyniku
result_frame = tk.Frame(root, bg="#e0e0e0", bd=2, relief="groove")
result_frame.pack(pady=10, padx=10, fill="both", expand=True)

result_label = tk.Label(result_frame, text="Wynik:", **label_style)
result_label.pack(pady=5)
result_text = tk.scrolledtext.ScrolledText(
    result_frame, width=60, height=10, font=("Arial", 10), bg="#ffffff"
)
result_text.pack(padx=5, pady=5, fill="both", expand=True)

# Uruchom aplikację
root.mainloop()