main: main.o block.o board.o keyboard.o server.o match.o screen.o
	g++ -O3 -o main main.o block.o board.o keyboard.o server.o match.o screen.o
block.o: block.cpp block.h
	g++ -c block.cpp
board.o: board.cpp board.h
	g++ -c board.cpp
keyboard.o: keyboard.cpp keyboard.h
	g++ -c keyboard.cpp
server.o: server.cpp server.h
	g++ -c server.cpp
match.o: match.cpp match.h
	g++ -c match.cpp
screen.o: screen.cpp screen.h
	g++ -c screen.cpp
main.o: main.cpp block.h board.h keyboard.h server.h match.h
	g++ -c main.cpp

clean:
	rm main *.o
