JAVAC = /usr/lib/jvm/java-8-openjdk-amd64/bin/javac
JAVA = /usr/lib/jvm/java-8-openjdk-amd64/bin/java
PORT = 1050

all: idlj compile_call compile_station compile_tube start_orbd start_station

idlj:
	idlj -fall Cell.idl
compile_call:
	$(JAVAC) Cell/*.java
compile_station:
	$(JAVAC) StationServer.java
compile_tube:
	$(JAVAC) Tube.java
start_orbd:
	orbd -ORBInitialPort $(PORT) -ORBInitialHost localhost &
start_station:
	$(JAVA) StationServer -ORBInitialPort $(PORT) -ORBInitialHost localhost &
start_tube1:
	$(JAVA) Tube -num 302 -ORBInitialPort $(PORT) -ORBInitialHost localhost
start_tube2:
	$(JAVA) Tube -num 439 -ORBInitialPort $(PORT) -ORBInitialHost localhost
start_tube3:
	$(JAVA) Tube -num 525 -ORBInitialPort $(PORT) -ORBInitialHost localhost