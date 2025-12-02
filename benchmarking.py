import ctypes
import statistics
import matplotlib.pyplot as plt


lib = ctypes.CDLL("./operaciones.so")

# Firmas de las funciones C que vamos a usar
lib.cpu_secuencial.restype = ctypes.c_double

lib.cpu_threading.argtypes = [ctypes.c_int]
lib.cpu_threading.restype = ctypes.c_double

lib.cpu_multiprocessing.argtypes = [ctypes.c_int]
lib.cpu_multiprocessing.restype = ctypes.c_double

lib.io_secuencial.restype = ctypes.c_double

lib.io_threading.argtypes = [ctypes.c_int]
lib.io_threading.restype = ctypes.c_double

lib.io_multiprocessing.argtypes = [ctypes.c_int]
lib.io_multiprocessing.restype = ctypes.c_double


# ----------------- Función auxiliar de medición -----------------
def medir(func, *args, repeticiones=5):
    """Ejecuta func(*args) varias veces y devuelve la mediana del tiempo."""
    tiempos = []
    for _ in range(repeticiones):
        t = func(*args)
        tiempos.append(t)
    return statistics.median(tiempos)


# ==============================================================
#   CPU-bound: Threading vs Multiprocessing vs Secuencial
# ==============================================================

workers = list(range(1, 21))

print("Midiendo CPU-bound...")

# Tiempo secuencial (no depende de workers)
t_seq_cpu = medir(lib.cpu_secuencial)
t_seq_cpu_line = [t_seq_cpu] * len(workers)

# Tiempos con hilos y procesos
t_thr_cpu = [medir(lib.cpu_threading, w) for w in workers]
t_mp_cpu  = [medir(lib.cpu_multiprocessing, w) for w in workers]

# Speedup
speedup_thr_cpu = [t_seq_cpu / t for t in t_thr_cpu]
speedup_mp_cpu  = [t_seq_cpu / t for t in t_mp_cpu]

print(f"CPU-bound: tiempo secuencial = {t_seq_cpu:.4f} s")
print("Speedup CPU-bound (Threading):")
print(speedup_thr_cpu)
print("Speedup CPU-bound (Multiprocessing):")
print(speedup_mp_cpu)


plt.figure(figsize=(8, 5))

plt.plot(workers, t_thr_cpu, "o-", label="Threading")
plt.plot(workers, t_mp_cpu, "s-", label="Multiprocessing")
plt.plot(workers, t_seq_cpu_line, "--", label="Secuencial")

plt.xlabel("Número de workers (hilos/procesos)")
plt.ylabel("Tiempo de ejecución (s)")
plt.title("CPU-bound: Threading vs Multiprocessing\n(tiempo vs número de workers)")
plt.grid(True)
plt.legend()
plt.tight_layout()


plt.savefig("cpu_bound_workers.png")
# plt.show()


# ==============================================================
#   I/O-bound: mediciones 
# ==============================================================

print("\nMidiendo I/O-bound...")

t_seq_io = medir(lib.io_secuencial)
t_thr_io = [medir(lib.io_threading, w) for w in workers]
t_mp_io  = [medir(lib.io_multiprocessing, w) for w in workers]

speedup_thr_io = [t_seq_io / t for t in t_thr_io]
speedup_mp_io  = [t_seq_io / t for t in t_mp_io]

print(f"I/O-bound: tiempo secuencial = {t_seq_io:.4f} s")
print("Speedup I/O-bound (Threading):")
print(speedup_thr_io)
print("Speedup I/O-bound (Multiprocessing):")
print(speedup_mp_io)


plt.figure(figsize=(8, 5))
plt.plot(workers, t_thr_io, "o-", label="Threading")
plt.plot(workers, t_mp_io, "s-", label="Multiprocessing")
plt.plot(workers, [t_seq_io] * len(workers), "--", label="Secuencial")
plt.xlabel("Número de workers (hilos/procesos)")
plt.ylabel("Tiempo de ejecución (s)")
plt.title("I/O-bound: Threading vs Multiprocessing\n(tiempo vs número de workers)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("io_bound_workers.png")
plt.show()
