# Разработка распределенного приложения на базе CORBA средствами языка программирования Java
Базовое задание представляет собой приложение, имитирующее работу соты мобильной телефонной связи при передаче коротких текстовых сообщений (SMS). Сота состоит из одной базовой станции и произвольного количества мобильных телефонных трубок и реализует две операции:

- регистрация трубки в соте (в базовой станции)
- пересылка строки текста от трубки к трубке через базовую станцию
Дополнительное задание: 
- передача мультимедийных сообщений в виде двоичных файлов


## Запуск приложения
1. Трансляция IDL-файла во вспомогательные файлы java: `make idlj`.
2. Компиляция сгенерированных java-файлов: `make compile_call`.
3. Компиляция серванта объекта "Базовая станция" и сервера: `make compile_station`.
4. Компиляция имитатора телефонной трубки: `make compile_tube`.
5. Запуск на выполнение службы имен orbd: `make start_orbd`.
6. Запуск на выполнение сервера: `make start_station`.
7. Запуск на выполнение клиента: `make start_tubeN`, где N = {1,2,3} для удобства тестирования (можно изменить в Makefile).

#### Примечания:
- пункты 1-5 объединены командой `make all`,
- путь к javac/java заданы в переменной BIN_PATH (если у вас отличается - заменить).
## Требования
- Java 8 и ниже,
- Linux.

---

Origin README: https://github.com/jexwerqwez