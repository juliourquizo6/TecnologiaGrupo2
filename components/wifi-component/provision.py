import logging
import sys
import os

# Añadir la ruta de la librería esp_prov si no está instalada
try:
    import esp_prov
except ImportError:
    sys.path.append("/home/joel/esp/esp-idf/tools")
    import esp_prov

import pytest
from pytest_embedded import Dut

logging.basicConfig(level=logging.INFO)

@pytest.fixture
def config(request):
    return 'security1'

# Función para realizar el provisioning
@pytest.mark.parametrize('config', ['security1'], indirect=True)
def provision_esp32(dut: Dut, sec_ver: int, ap_ssid: str, ap_password: str, config) -> None:
    # Obtener el nombre del dispositivo BLE desde la salida del ESP32
    devname = "PROV_01884"
    logging.info('Dispositivo BLE del DUT : {}'.format(devname))

    # Configurar el modo de provisión (BLE o SoftAP)
    provmode = 'softap'

    # Definir los parámetros de seguridad (dependiendo de la versión)
    if sec_ver == 1:
        pop = 'abcd1234'  # Pop string para seguridad v1
        sec2_username = None
        sec2_password = None
        security = esp_prov.get_security(sec_ver, sec2_username, sec2_password, pop, verbose=True)
    elif sec_ver == 2:
        pop = None
        sec2_username = 'wifiprov'
        sec2_password = 'abcd1234'
        security = esp_prov.get_security(sec_ver, sec2_username, sec2_password, pop, verbose=True)

    # Verificar que se ha configurado correctamente la seguridad
    if security is None:
        raise RuntimeError('No se pudo configurar la seguridad')

    logging.info('Obteniendo transporte')
    transport = esp_prov.get_transport(provmode, devname)
    if transport is None:
        raise RuntimeError('No se pudo obtener el transporte')

    logging.info('Verificando versión de protocolo')
    if not esp_prov.version_match(transport, 'v1.1'):
        raise RuntimeError('Incompatibilidad de versión de protocolo')

    logging.info('Verificando capacidad de escaneo')
    if not esp_prov.has_capability(transport, 'wifi_scan'):
        raise RuntimeError('Capacidad de escaneo no disponible')

    logging.info('Iniciando la sesión')
    if not esp_prov.establish_session(transport, security):
        raise RuntimeError('No se pudo establecer la sesión')

    logging.info('Enviando credenciales Wi-Fi al ESP32')
    if not esp_prov.send_wifi_config(transport, security, ap_ssid, ap_password):
        raise RuntimeError('No se pudieron enviar las credenciales de Wi-Fi')

    logging.info('Aplicando configuración')
    if not esp_prov.apply_wifi_config(transport, security):
        raise RuntimeError('No se pudo aplicar la configuración')

    logging.info('Esperando que el ESP32 se conecte a Wi-Fi')
    if not esp_prov.wait_wifi_connected(transport, security):
        raise RuntimeError('El proceso de provisión falló')

    logging.info('Proceso de provisión completado con éxito')

# Ejemplo de ejecución de prueba con parámetros de seguridad 1
@pytest.mark.esp32
@pytest.mark.generic
@pytest.mark.parametrize('config', ['security1',], indirect=True)
def test_wifi_prov_mgr_sec1(dut: Dut, config) -> None:
    ap_ssid = 'MiRedWiFi'
    ap_password = 'MiPassword2024'
    provision_esp32(dut, sec_ver=1, ap_ssid=ap_ssid, ap_password=ap_password, config=config)

# Ejemplo de ejecución de prueba con parámetros de seguridad 2
@pytest.mark.esp32
@pytest.mark.generic
def test_wifi_prov_mgr_sec2(dut: Dut, config) -> None:
    ap_ssid = 'MIOT'
    ap_password = 'MIOT_WIFI_2024!'
    provision_esp32(dut, sec_ver=2, ap_ssid=ap_ssid, ap_password=ap_password, config=config)
