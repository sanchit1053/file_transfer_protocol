all: c1 c2 c3 c4 c5

c1: client-phase1.cpp utils.cpp
	g++ -std=c++17 client-phase1.cpp  utils.cpp -o client-phase1 -lcrypto -lssl

c2: client-phase2.cpp utils.cpp
	g++ -std=c++17 client-phase2.cpp utils.cpp -o client-phase2 -lcrypto -lssl

c3: client-phase3.cpp utils.cpp
	g++ -std=c++17 client-phase3.cpp utils.cpp -o client-phase3 -lcrypto -lssl

c4: client-phase4.cpp utils.cpp
	g++ -std=c++17 client-phase4.cpp utils.cpp -o client-phase4 -lcrypto -lssl

c5: client-phase5.cpp utils.cpp
	g++ -std=c++17 client-phase5.cpp utils.cpp -o client-phase5 -lcrypto -lssl


