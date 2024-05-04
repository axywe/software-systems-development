import Cell.*;
import java.io.*;
import org.omg.CORBA.*;
import org.omg.CosNaming.*;
import org.omg.PortableServer.*;
import org.omg.PortableServer.POA;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Base64;

// Класс вызова телефонной трубки
class TubeCallbackServant extends TubeCallbackPOA {
  private String myNum; // Номер трубки
  public void receiveFile(String fromNum, String fileContentBase64) {
    System.out.println("Трубка " + myNum + ": получен файл от " + fromNum);
    // Здесь вы можете добавить логику для сохранения файла на диск или обработки его содержимого
    // Например, сохраняем файл на диск
    byte[] fileContent = Base64.getDecoder().decode(fileContentBase64);
    try {
      Files.write(Paths.get("received_file"), fileContent);
      System.out.println("Файл сохранен на диск.");
    } catch (IOException e) {
      System.out.println("Ошибка при сохранении файла: " + e.getMessage());
    }
  }

  // Конструктор класса
  public TubeCallbackServant(String num) {
    myNum = num;
  };

  // Метод обработки принятого сообщения
  public int sendSMS(String fromNum, String message) {
    System.out.println(myNum + ": принято сообщение от " + fromNum + ": " + message);
    return (0);
  };

  // Метод, возвращающий номер трубки
  public String getNum() {
    return (myNum);
  };
};

// Класс, используемый для создания потока управления
class ORBThread extends Thread {
  private ORB orb;

  // Конструктор класса
  public ORBThread(ORB orb) {
    this.orb = orb;
  };

  // Метод запуска потока
  public void run() {
    orb.run();
  };
};

// Класс, имитирующий телефонную трубку
public class Tube {
  public static void main(String args[]) {
    try {
      String myNum = null;
      for (int i = 0; i < args.length; i++) {
        if (args[i].equals("-num") && i + 1 < args.length) // Считывание номера трубки
          myNum = args[i + 1];
      }
      if (myNum == null) {
        System.out.println("Необходимо указать номер трубки используя флаг '-num'.");
        return;
      }
      // Создание и инициализация ORB
      ORB orb = ORB.init(args, null);

      //Создание серванта для IDL-интерфейса TubeCallback
      POA rootPOA = POAHelper.narrow(orb.resolve_initial_references("RootPOA"));
      rootPOA.the_POAManager().activate();
      TubeCallbackServant listener = new TubeCallbackServant(myNum);
      rootPOA.activate_object(listener);
      // Получение ссылки на сервант
      TubeCallback ref = TubeCallbackHelper.narrow(rootPOA.servant_to_reference(listener));

      // Получение контекста именования
      org.omg.CORBA.Object objRef = orb.resolve_initial_references("NameService");
      //   NamingContext ncRef = NamingContextHelper.narrow(objRef);

      //   // Преобразование имени базовой станции в объектную ссылку
      //   NameComponent nc = new NameComponent("BaseStation", "");
      //   NameComponent path[] = {nc};
      NamingContextExt ncRef = NamingContextExtHelper.narrow(objRef);

      Station stationRef = StationHelper.narrow(ncRef.resolve_str("BaseStation"));

      // Регистрация трубки в базовой станции
      if (stationRef.register(ref, myNum) == 1) {
        System.out.println("Трубка " + myNum + " зарегистрирована в базовой станции");
      } else {
        System.out.println("Ошибка регистрации трубки " + myNum);
        return;
      }
      // Запуск ORB в отдельном потоке управления
      // для прослушивания вызовов трубки
      ORBThread orbThr = new ORBThread(orb);
      orbThr.start();

      // Бесконечный цикл чтения строк с клавиатуры и отсылки их
      // базовой станции
      BufferedReader inpt = new BufferedReader(new InputStreamReader(System.in));
    String msg;
    System.out.println("Введите номер получателя и сообщение в формате 'номер:сообщение' или 'номер:file:путь_к_файлу':");
    while (true) {
      msg = inpt.readLine();
      if (msg.isEmpty())
        break;
      String[] parts = msg.split(":", 2);
      if (parts.length == 2) {
        // Если сообщение начинается с 'file:', то передаем файл
        if (parts[1].startsWith(" file:")) {
          String filePath = parts[1].substring(6); // Получаем путь к файлу
          byte[] fileContent = Files.readAllBytes(Paths.get(filePath)); // Читаем содержимое файла
          String fileContentBase64 = Base64.getEncoder().encodeToString(fileContent);
          int result = stationRef.sendFile(myNum, parts[0], fileContentBase64); // Отправляем файл
          if (result == 0)
            System.out.println("Ошибка: номер " + parts[0] + " не существует.");
        } else {
          int result = stationRef.sendSMS(myNum, parts[0], parts[1]);
          if (result == 0)
            System.out.println("Ошибка: номер " + parts[0] + " не существует.");
        }
      } else {
        System.out.println("Неверный формат ввода");
      }
    }
      orb.shutdown(true);
      orbThr.join(); // Дождаться завершения работы потока ORB
    } catch (Exception e) {
      e.printStackTrace();
    };
  };
};
