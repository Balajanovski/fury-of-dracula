########################################################################
# COMP2521 20T2 // the Fury of Dracula // the View
# view/Makefile: build tests for GameView/HunterView/DraculaView
#
# You can modify this if you add additional files
#
# 2018-12-31	v1.0	Team Dracula <cs2521@cse.unsw.edu.au>
# 2020-07-10    v2.0    Team Dracula <cs2521@cse.unsw.edu.au>
#
########################################################################

CC = gcc

# For debugging, set the c flags to
# CFLAGS = -Wall -g -pthread

# For our submissions, set the c flags to
CFLAGS = -Wall -pthread -O3 -DNDEBUG

LDFLAGS = -lz -pthread
BINS = testGameView testHunterView testDraculaView testMap dracula hunter testKTree

OBJS = GameView.o Map.o Places.o LocationDynamicArray.o DraculaTrail.o Queue.o MoveSet.o kTree.o

LIBS = -lm -lrt -lpthread

all: $(BINS)

dracula: playerDracula.o dracula.o DraculaView.o  $(OBJS) $(LIBS)
hunter: playerHunter.o hunter.o HunterView.o Probability.o $(OBJS) $(LIBS)

playerDracula.o: player.c dracula.h Game.h DraculaView.h GameView.h Places.h
	$(CC) $(CFLAGS) -DI_AM_DRACULA -c $< -o $@
playerHunter.o: player.c hunter.h Game.h HunterView.h GameView.h Places.h
	$(CC) $(CFLAGS) -c $< -o $@

dracula.o: dracula.c dracula.h DraculaView.o $(OBJS)
hunter.o: hunter.c hunter.h HunterView.o $(OBJS)

testKTree: kTree.o
testKTree.o: kTree.o

testGameView: testGameView.o testUtils.o GameView.o Map.o Places.o LocationDynamicArray.o DraculaTrail.o Queue.o MoveSet.o
testGameView.o: testGameView.c GameView.h Map.h Places.h Game.h DraculaTrail.h

testHunterView: testHunterView.o testUtils.o HunterView.o GameView.o Map.o Places.o LocationDynamicArray.o DraculaTrail.o Queue.o MoveSet.o
testHunterView.o: testHunterView.c HunterView.h GameView.h Map.h Places.h Game.h DraculaTrail.h

testDraculaView: testDraculaView.o testUtils.o DraculaView.o GameView.o Map.o Places.o LocationDynamicArray.o DraculaTrail.o Queue.o MoveSet.o
testDraculaView.o: testDraculaView.c DraculaView.h GameView.h Map.h Places.h Game.h DraculaTrail.h

testMap: testMap.o Map.o Places.o
testMap.o: testMap.c Map.h Places.h

Places.o: Places.c Places.h Game.h
Map.o: Map.c Map.h Places.h Game.h
GameView.o:	GameView.c GameView.h Game.h DraculaTrail.o DraculaTrail.h LocationDynamicArray.o LocationDynamicArray.h Queue.o MoveSet.o
HunterView.o: HunterView.c HunterView.h Game.h GameView.o GameView.h
DraculaView.o: DraculaView.c DraculaView.h Game.h GameView.o GameView.h MoveSet.o
testUtils.o: testUtils.c Places.h Game.h
DraculaTrail.o: DraculaTrail.c DraculaTrail.h Places.h DraculaMove.h
LocationDynamicArray.o: LocationDynamicArray.c LocationDynamicArray.h Places.h
Queue.o: Queue.c Queue.h Places.h
MoveSet.o: MoveSet.c MoveSet.h Places.h
kTree.o: kTree.c kTree.h
Probability.o: Probability.c Probability.h

.PHONY: clean
clean:
	-rm -f ${BINS} *.o core
