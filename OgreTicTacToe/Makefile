#make executable binary
OgreTest:	OgreTest.o t2.o
	g++ -g -o OgreTest OgreTest.o t2.o -lode -lOgreMain -lOIS -lpthread -L ~/ode-0.15.2/ode/src/.libs

t2.o: t2.h t2.cpp
	g++ -c t2.cpp -I ~/ode-0.15.2/include

#make object file - maybe -I
OgreTest.o:	OgreTest.cpp t2.h
	g++ -g -c -o OgreTest.o OgreTest.cpp -I ~/ode-0.15.2/include
