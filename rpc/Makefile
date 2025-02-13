all:server client apigateway
client:client.pb_skeleton.o signup.pb.o
	g++ $^ -o $@ -lsrpc -lworkflow -lcrypt -lprotobuf -lppconsul -lwfrest -lssl -lcrypto
client.pb_skeleton.o:client.pb_skeleton.cc
	g++ -c $^ -o $@
signup.pb.o:signup.pb.cc
	g++ -c $^ -o $@

server:server.pb_skeleton.o signup.pb.o
	g++ $^ -o $@ -lsrpc -lworkflow -lcrypt -lprotobuf -lppconsul -lwfrest -lssl -lcrypto
server.pb_skeleton.o:server.pb_skeleton.cc
	g++ -c $^ -o $@

apigateway:main.o signup.pb.o
	g++ $^ -o $@ -lsrpc -lworkflow -lcrypt -lprotobuf -lppconsul -lwfrest -lssl -lcrypto
main.o:main.cc
	g++ -c $^ -o $@