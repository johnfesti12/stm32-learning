import serial
import json
import time

COM_PORT = 'COM8'
BAUD_RATE = 115200

try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout = 1)
    print(f"Успешно подключено к {COM_PORT}. Ожидаем данные...")
    print("-"*30)

    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()

            #Пропускаем пустые строки и сообщения от printf
            if not line of not line.startswith('{'):
                    continue

            try:
                #Парсим JSON строку в словарь Python
                data = json.loads(line)
                roll = data.get('roll', 0.0)
                pitch = data.get('pitch', 0.0)

                #Выводим красиво отформатированные данные
                print(f"Крен (Roll): {roll:6.1f}° | Тангаж (Pitch): {pitch:6.1f}°")

            except json.JSONDecodeError:
                #Если строка битая(помехи), пропускаем
                pass

    except serial.SerialException as e:
        print(f"Ошибка порта: {e}\nПроверьте, что терминал закрыт!")
    except KeyboardInterrupt:
        print("\nЗавершение работы.")
        if 'ser' in locals() and ser.is_open:
            ser.close()
