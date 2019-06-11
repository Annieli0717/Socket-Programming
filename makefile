all: serverA.cpp serverB.cpp aws.cpp client.cpp monitor.cpp
	g++ -o serverA serverA.cpp

	g++ -o serverB serverB.cpp

	g++ -o aws aws.cpp
	
	g++ -o client client.cpp	

	g++ -o monitor monitor.cpp

.PHONY: serverA
serverA: 
	./serverA

.PHONY: serverB
serverB:
	./serverB

.PHONY: AWS
AWS:
	./aws

.PHONY: client
client:
	./client

.PHONY: monitor
monitor:
	./monitor

