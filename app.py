import tkinter as tk
from tkinter import messagebox
import subprocess


def calculate():
    # Pobierz wartości z interfejsu
    mode = mode_var.get()
    nodes_str = nodes_entry.get()
    x_str = x_entry.get()
    y_str = y_entry.get()

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

    # Sprawdź, czy liczba wartości zgadza się z liczbą węzłów
    if len(x_values) != nodes or len(y_values) != nodes:
        tk.messagebox.showerror(
            "Błąd", "Liczba wartości x i y musi zgadzać się z liczbą węzłów."
        )
        return

    # Konwertuj wartości na liczby zmiennoprzecinkowe
    try:
        x_floats = [float(x) for x in x_values]
        y_floats = [float(y) for y in y_values]
    except ValueError:
        tk.messagebox.showerror("Błąd", "Wartości x i y muszą być liczbami.")
        return

    # Zapisz dane do pliku input.txt
    with open("input.txt", "w") as f:
        f.write(str(mode) + "\n")
        f.write(str(nodes) + "\n")
        f.write(" ".join(map(str, x_floats)) + "\n")
        f.write(" ".join(map(str, y_floats)) + "\n")

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
        result_var.set("Wynik: " + result)
    except FileNotFoundError:
        tk.messagebox.showerror("Błąd", "Nie znaleziono pliku wyjściowego.")
    except Exception as e:
        tk.messagebox.showerror("Błąd", str(e))


# Utwórz główne okno
root = tk.Tk()
root.title("Aplikacja obliczeniowa")

# Wybór trybu
mode_label = tk.Label(root, text="Wybierz tryb:")
mode_label.pack()

mode_var = tk.IntVar()
mode_var.set(1)  # Domyślnie tryb 1
mode1_radio = tk.Radiobutton(root, text="Tryb 1", variable=mode_var, value=1)
mode1_radio.pack()
mode2_radio = tk.Radiobutton(root, text="Tryb 2", variable=mode_var, value=2)
mode2_radio.pack()
mode3_radio = tk.Radiobutton(root, text="Tryb 3", variable=mode_var, value=3)
mode3_radio.pack()

# Liczba węzłów
nodes_label = tk.Label(root, text="Wprowadź liczbę węzłów:")
nodes_label.pack()
nodes_entry = tk.Entry(root)
nodes_entry.pack()

# Wartości x
x_label = tk.Label(root, text="Wprowadź wartości x (oddzielone spacjami):")
x_label.pack()
x_entry = tk.Entry(root, width=50)
x_entry.pack()

# Wartości y
y_label = tk.Label(root, text="Wprowadź wartości y (oddzielone spacjami):")
y_label.pack()
y_entry = tk.Entry(root, width=50)
y_entry.pack()

# Przycisk obliczeń
calculate_button = tk.Button(root, text="Oblicz", command=calculate)
calculate_button.pack()

# Etykieta wyniku
result_var = tk.StringVar()
result_var.set("Wynik: ")
result_label = tk.Label(root, textvariable=result_var)
result_label.pack()

# Uruchom aplikację
root.mainloop()
