import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import subprocess

# Stylizacja kolorów
BG_COLOR = "#f5f6fa"
ACCENT_COLOR = "#487eb0"
BUTTON_COLOR = "#40739e"
TEXT_COLOR = "#2f3640"
FONT_NAME = "Segoe UI"

def calculate():
    mode = mode_var.get()
    nodes_str = nodes_entry.get()
    x_str = x_entry.get()
    y_str = y_entry.get()
    xx_str = xx_entry.get()

    try:
        nodes = int(nodes_str)
        if nodes <= 0:
            raise ValueError
    except ValueError:
        messagebox.showerror("Błąd", "Liczba węzłów musi być dodatnią liczbą całkowitą.")
        return

    x_values = x_str.split()
    y_values = y_str.split()
    xx_values = xx_str.split()

    # Walidacja liczby wartości
    if mode == 1:
        required = nodes
        if len(x_values) != required or len(y_values) != required:
            messagebox.showerror("Błąd", f"W trybie 1 wymagane {required} wartości x i y")
            return
    elif mode == 2:
        required = 2 * nodes
        if len(x_values) != required or len(y_values) != required:
            messagebox.showerror("Błąd", f"W trybie 2 wymagane {required} wartości x i y")
            return
    elif mode == 3:
        required = 4 * nodes
        if len(x_values) != required or len(y_values) != required:
            messagebox.showerror("Błąd", f"W trybie 3 wymagane {required} wartości x i y")
            return
    else:
        messagebox.showerror("Błąd", "Nieobsługiwany tryb")
        return

    # Walidacja wartości xx
    try:
        if mode == 1:
            if len(xx_values) != 1: raise ValueError
            xx = [float(xx_values[0])]
        elif mode == 2:
            if len(xx_values) != 2: raise ValueError
            xx = list(map(float, xx_values))
        elif mode == 3:
            if len(xx_values) != 4: raise ValueError
            xx = list(map(float, xx_values))
    except ValueError:
        messagebox.showerror("Błąd", "Nieprawidłowy format punktu xx dla wybranego trybu")
        return

    # Zapisz dane do pliku
    with open("input.txt", "w") as f:
        f.write(f"{mode}\n{nodes}\n")
        f.write(" ".join(x_values) + "\n")
        f.write(" ".join(y_values) + "\n")
        f.write(" ".join(map(str, xx)) + "\n")

    # Uruchom obliczenia
    try:
        subprocess.run(["./main"], check=True)
    except Exception as e:
        messagebox.showerror("Błąd", f"Błąd obliczeń: {str(e)}")
        return

    # Wyświetl wyniki
    try:
        with open("output.txt", "r") as f:
            result = f.read()
        result_text.delete(1.0, tk.END)
        result_text.insert(tk.END, "Wyniki:\n" + result)
    except Exception as e:
        messagebox.showerror("Błąd", f"Błąd odczytu wyników: {str(e)}")

def show_info():
    info = """
    FORMAT DANYCH WEJŚCIOWYCH:

    Tryb 1 (Zwykły):
    - x: pojedyncze wartości (np. 1 2 3)
    - y: pojedyncze wartości (np. 2 3 5)
    - xx: pojedyncza wartość (np. 2.5)

    Tryb 2 (Przedziały pojedyncze):
    - x: pary przedziałów (np. 1.9 2.1 2.9 3.1)
    - y: pary przedziałów (np. 1.9 2.1 2.9 3.1)
    - xx: para przedziałów (np. 2.4 2.6)

    Tryb 3 (Podwójne przedziały):
    - x: czwórki wartości [[lo1, hi1], [lo2, hi2]] (np. 1.8 2.0 2.2 2.4 2.8 3.0 3.2 3.4)
    - y: czwórki wartości [[lo1, hi1], [lo2, hi2]] 
    - xx: czwórka wartości (np. 2.3 2.4 2.6 2.7)
    """
    messagebox.showinfo("Instrukcja", info)

# Główne okno
root = tk.Tk()
root.title("Spline Calculator - Interpolacja przedziałowa")
root.geometry("900x750")
root.configure(bg=BG_COLOR)

# Styl dla widgetów
style = ttk.Style()
style.theme_use('clam')
style.configure('TFrame', background=BG_COLOR)
style.configure('TLabel', background=BG_COLOR, font=(FONT_NAME, 10), foreground=TEXT_COLOR)
style.configure('TButton', font=(FONT_NAME, 10, 'bold'), borderwidth=1)
style.map('TButton', 
          foreground=[('active', BG_COLOR), ('!active', BG_COLOR)],
          background=[('active', BUTTON_COLOR), ('!active', ACCENT_COLOR)])

# Nagłówek
header_frame = ttk.Frame(root)
header_frame.pack(pady=20, fill='x')
tk.Label(header_frame, 
        text="SPLINE CALCULATOR", 
        font=(FONT_NAME, 18, 'bold'), 
        fg=ACCENT_COLOR, 
        bg=BG_COLOR).pack()

# Panel trybów
mode_frame = ttk.LabelFrame(root, text=" Tryb obliczeń ", padding=15)
mode_frame.pack(padx=20, pady=10, fill='x')

mode_var = tk.IntVar(value=1)
modes = [
    ("Tryb 1 - Standardowy", 1),
    ("Tryb 2 - Pojedyncze przedziały", 2),
    ("Tryb 3 - Podwójne przedziały", 3)
]

for text, val in modes:
    ttk.Radiobutton(mode_frame, 
                   text=text, 
                   variable=mode_var, 
                   value=val,
                   style='Toolbutton').pack(side='left', padx=10, pady=5)

# Panel danych wejściowych
input_frame = ttk.LabelFrame(root, text=" Dane wejściowe ", padding=15)
input_frame.pack(padx=20, pady=15, fill='x')

def create_input_row(frame, label, row):
    tk.Label(frame, 
            text=label, 
            font=(FONT_NAME, 10, 'bold'), 
            bg=BG_COLOR, 
            fg=TEXT_COLOR).grid(row=row, column=0, sticky='w', padx=5, pady=8)
    entry = ttk.Entry(frame, width=60, font=(FONT_NAME, 10))
    entry.grid(row=row, column=1, padx=5, pady=8, sticky='ew')
    return entry

nodes_entry = create_input_row(input_frame, "Liczba węzłów:", 0)
x_entry = create_input_row(input_frame, "Wartości x:", 1)
y_entry = create_input_row(input_frame, "Wartości y:", 2)
xx_entry = create_input_row(input_frame, "Punkt xx:", 3)

# Przyciski akcji
button_frame = ttk.Frame(root)
button_frame.pack(pady=15)

ttk.Button(button_frame, 
          text="Oblicz", 
          command=calculate, 
          style='TButton').pack(side='left', padx=10)
ttk.Button(button_frame, 
          text="ℹ️ Instrukcja", 
          command=show_info, 
          style='TButton').pack(side='left', padx=10)

# Panel wyników
result_frame = ttk.LabelFrame(root, text=" Wyniki ", padding=15)
result_frame.pack(padx=20, pady=10, fill='both', expand=True)

result_text = scrolledtext.ScrolledText(
    result_frame,
    wrap=tk.WORD,
    font=(FONT_NAME, 10),
    bg='white',
    padx=10,
    pady=10,
    width=80,
    height=15
)
result_text.pack(fill='both', expand=True)

# Stopka
footer_frame = ttk.Frame(root)
footer_frame.pack(pady=10)
tk.Label(footer_frame, 
        text="© 2024 Spline Calculator | Wersja 3.0", 
        font=(FONT_NAME, 8), 
        fg='#7f8fa6', 
        bg=BG_COLOR).pack()

# Responsywność
for child in input_frame.winfo_children():
    child.grid_configure(padx=10, pady=5)
input_frame.columnconfigure(1, weight=1)

root.mainloop()