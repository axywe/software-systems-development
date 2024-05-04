import Cell.*;
import java.util.HashMap;
import java.util.Properties;
import org.omg.CORBA.*;
import org.omg.CosNaming.*;
import org.omg.CosNaming.NamingContextPackage.*;
import org.omg.PortableServer.*;
import org.omg.PortableServer.POA;
import java.util.Base64;

// Класс, реализующий IDL-интерфейс базовой станции
class StationServant extends StationPOA {
  // Вместо представленных ниже двух переменных здесь
  // должен быть список пар "номер - объектная ссылка"
  private HashMap<String, TubeCallback> registerTubes = new HashMap<>();

  public int sendFile(String fromNum, String toNum, String fileContentBase64) {
  if (!registerTubes.containsKey(toNum)) { // если номер получателя не найден
    System.out.println("Станция: трубка с номером " + toNum + " не найдена.");
    return 0;
  }
  TubeCallback receiver = registerTubes.get(toNum);
  try {
    receiver.receiveFile(fromNum, fileContentBase64);
    System.out.println("Станция: трубка " + fromNum + " посылает файл " + toNum);
    return 1;
  } catch (Exception e) {
    System.out.println("Ошибка при передаче файла: " + e.getMessage());
    return 0;
  }
}

  // Метод регистрации трубки в базовой станции
  public int register(TubeCallback objRef, String phoneNum) {
    if (!registerTubes.containsKey(phoneNum)) { // если трубка ещё не зарегистрирована
      registerTubes.put(phoneNum, objRef);
      System.out.println("Станция: зарегистрирована трубка " + phoneNum);
      return 1;
    }
    System.out.println("Станция: трубка с номером " + phoneNum + " уже зарегистрирована.");
    return 0;
  };

  // Метод пересылки сообщения от трубки к трубке
  public int sendSMS(String fromNum, String toNum, String message) {
    if (!registerTubes.containsKey(toNum)) { // если номер получателя не найден
      System.out.println("Станция: трубка с номером " + toNum + " не найдена.");
      return 0;
    }
    TubeCallback reciever = registerTubes.get(toNum);
    reciever.sendSMS(fromNum, message);
    System.out.println("Станция: трубка " + fromNum + " посылает сообщение " + toNum);
    return 1;
  };
};

// Класс, реализующий сервер базовой станции
public class StationServer {
  public static void main(String args[]) {
    try {
      // Создание и инициализация ORB
      ORB orb = ORB.init(args, null);

      // Получение ссылки и активирование POAManager
      POA rootpoa = POAHelper.narrow(orb.resolve_initial_references("RootPOA"));
      rootpoa.the_POAManager().activate();

      // Создание серванта для CORBA-объекта "базовая станция"
      StationServant servant = new StationServant();

      // Получение объектной ссылки на сервант
      org.omg.CORBA.Object ref = rootpoa.servant_to_reference(servant);
      Station sref = StationHelper.narrow(ref);

      org.omg.CORBA.Object objRef = orb.resolve_initial_references("NameService");
      NamingContextExt ncRef = NamingContextExtHelper.narrow(objRef);

      // Связывание объектной ссылки с именем
      String name = "BaseStation";
      NameComponent path[] = ncRef.to_name(name);
      ncRef.rebind(path, sref);

      System.out.println("Сервер готов и ждет ...");

      // Ожидание обращений от клиентов (трубок)
      orb.run();
    } catch (Exception e) {
      System.err.println("Ошибка: " + e);
      e.printStackTrace(System.out);
    };
  };
};
