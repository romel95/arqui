#!/usr/bin/env python
from socket import AF_INET, SOCK_DGRAM
import datetime
import threading
import socket
import struct
import time   # para medir tiempos

servidores_ntp = [
    "0.uk.pool.ntp.org",    # Londres(Reino Unido)
    "1.es.pool.ntp.org",    # Madrid (España)
    "0.us.pool.ntp.org",    # Nueva York(Estados Unidos)
    "0.hk.pool.ntp.org",    # Hong Kong
    "0.jp.pool.ntp.org"     # Tokyo(Japón)
]

"""
Función: get_ntp_time
Descripción: Imprime la  fecha-hora actual en un país determinado
Entrada: Cualquiera de las URLs definidas en la lista servidores_ntp
Salida: Retorna la fecha-hora(timestamp) en formato datetime.datetime, también la imprime
IMPORTANTE: NO modifique esta funcion 
"""
def get_ntp_time(host):
    timezone_dict = {'uk': ['UK', 0 * 3600], 'es': ['España', 1 * 3600],
                     'hk': ['Hong Kong', 8 * 3600], 'jp': ['Japón', 9 * 3600],
                     'us': ['Estados Unidos', -5*3600]}
    key = ''
    port = 123
    buf = 1024
    address = (host, port)
    msg = b'\x1b' + 47 * b'\0'

    # reference time (in seconds since 1900-01-01 00:00:00)
    TIME1970 = 2208988800  # 1970-01-01 00:00:00
    # connect to server
    client = socket.socket(AF_INET, SOCK_DGRAM)
    client.sendto(msg, address)
    msg, address = client.recvfrom(buf)
    t = struct.unpack("!12I", msg)[10]
    t -= TIME1970
    client.close()

    for each_key in timezone_dict:
        if each_key in host:
            key = each_key
            break
    print(f"Hora en {timezone_dict[key][0]}: {datetime.datetime.utcfromtimestamp(t + timezone_dict[key][1])}")
    return datetime.datetime.utcfromtimestamp(t + timezone_dict[key][1])

# =========================
#   FUNCIONES AUXILIARES
# =========================

def tiempo_faltante_para_apertura(dt_local):
    """
    Calcula cuánto falta para la PRÓXIMA apertura de la bolsa a las 08:00am
    en ese país (tomando dt_local como la hora local actual).
    """
    apertura_hoy = dt_local.replace(hour=8, minute=0, second=0, microsecond=0)
    if dt_local <= apertura_hoy:
        # Todavía no son las 8am, abre hoy
        delta = apertura_hoy - dt_local
    else:
        # Ya pasó la apertura de hoy, consideramos mañana 8am
        apertura_manana = apertura_hoy + datetime.timedelta(days=1)
        delta = apertura_manana - dt_local
    return delta

def obtener_nombre_pais(host):
    """
    Devuelve el nombre del país en función del host NTP.
    Coincide con los comentarios de la lista servidores_ntp.
    """
    if "uk" in host:
        return "Reino Unido"
    elif "es" in host:
        return "España"
    elif "us" in host:
        return "Estados Unidos"
    elif "hk" in host:
        return "Hong Kong"
    elif "jp" in host:
        return "Japón"
    else:
        return host  # por si acaso

# =========================
#   PARTE (a) – SECUENCIAL
# =========================

def bolsa_mas_proxima_secuencial():
    """
    Recorre la lista servidores_ntp SECUENCIALMENTE.
    Para cada servidor:
      - Obtiene la hora local con get_ntp_time()
      - Calcula cuánto falta para las 08:00am
    Al final imprime qué país tiene la bolsa más próxima a abrir
    y el tiempo de ejecución de esta función.
    """
    inicio = time.perf_counter()

    mejor_host = None
    mejor_delta = None

    for host in servidores_ntp:
        print(f"\n[SECUENCIAL] Consultando servidor: {host}")
        dt_local = get_ntp_time(host)  # imprime la hora y retorna datetime
        delta = tiempo_faltante_para_apertura(dt_local)

        if (mejor_delta is None) or (delta < mejor_delta):
            mejor_delta = delta
            mejor_host = host

    fin = time.perf_counter()
    tiempo_total = fin - inicio

    if mejor_host is not None:
        pais = obtener_nombre_pais(mejor_host)
        print("\n========== RESULTADO SECUENCIAL ==========")
        print(f"La bolsa más próxima a abrir es la de: {pais}")
        print(f"Servidor NTP: {mejor_host}")
        print(f"Tiempo faltante para abrir: {mejor_delta}")
        print(f"Tiempo de ejecución (secuencial): {tiempo_total:.4f} segundos")
        print("=========================================\n")

    return tiempo_total

# =========================
#   PARTE (b) – CON THREADS
# =========================

def trabajador_ntp(host, resultados, lock):
    """
    Función que ejecuta cada thread:
     - Llama a get_ntp_time(host)
     - Calcula el tiempo faltante para las 08:00
     - Guarda el resultado en el diccionario compartido 'resultados'
       usando un lock para evitar condiciones de carrera.
    """
    print(f"\n[THREAD] Consultando servidor: {host}")
    dt_local = get_ntp_time(host)
    delta = tiempo_faltante_para_apertura(dt_local)

    with lock:
        resultados[host] = delta

def bolsa_mas_proxima_con_threads():
    """
    Hace lo mismo que la parte (a) pero usando un THREAD por servidor NTP.
    Cuando todos terminan, calcula qué bolsa está más próxima a abrir
    y muestra el tiempo de ejecución total.
    """
    inicio = time.perf_counter()

    resultados = {}            # host -> delta
    lock = threading.Lock()
    hilos = []

    # Crear e iniciar un thread por servidor
    for host in servidores_ntp:
        t = threading.Thread(target=trabajador_ntp,
                             args=(host, resultados, lock))
        t.start()
        hilos.append(t)

    # Esperar que terminen todos los threads
    for t in hilos:
        t.join()

    # Elegir el host con menor delta
    mejor_host = None
    mejor_delta = None

    for host, delta in resultados.items():
        if (mejor_delta is None) or (delta < mejor_delta):
            mejor_delta = delta
            mejor_host = host

    fin = time.perf_counter()
    tiempo_total = fin - inicio

    if mejor_host is not None:
        pais = obtener_nombre_pais(mejor_host)
        print("\n======= RESULTADO CON THREADS =======")
        print(f"La bolsa más próxima a abrir es la de: {pais}")
        print(f"Servidor NTP: {mejor_host}")
        print(f"Tiempo faltante para abrir: {mejor_delta}")
        print(f"Tiempo de ejecución (threads): {tiempo_total:.4f} segundos")
        print("=====================================\n")

    return tiempo_total

# =========================
#   MAIN: comparación
# =========================

if __name__ == '__main__':
    # Parte (a)
    t_secuencial = bolsa_mas_proxima_secuencial()

    # Parte (b)
    t_threads = bolsa_mas_proxima_con_threads()

    # Comparación pedida en el enunciado
    print("******** COMPARACIÓN FINAL ********")
    if t_threads < t_secuencial:
        print("La versión CON THREADS fue más rápida que la SECUENCIAL.")
    elif t_threads > t_secuencial:
        print("La versión SECUENCIAL fue más rápida que la CON THREADS.")
    else:
        print("Ambas versiones tuvieron tiempos de ejecución muy similares.")
    print("***********************************")
