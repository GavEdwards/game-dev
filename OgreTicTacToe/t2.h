
#include <ode/ode.h>

// OGRE headers
#include <OGRE/Ogre.h>
#include <OIS/OIS.h>
#include <iostream>

#include <string>


#define SPACES_PER_BOARD 9


class Player
{
public:
  Player(char *_name){this->name = _name;}
  ~Player(){}
  char* getName(){return name;}
  dGeomID ODEgeomID;
  Ogre::SceneNode *OgreNode;
  Ogre::Entity *OgreEntity;
private:
  char *name;
};


class Space
{
public:
  Space(int x, int z, int y, dSpaceID space);
  ~Space(){ delete state;}
  void setPosition(int x, int z, int y, double margin);
  bool setSpace();
  bool clearSpace();
  Player checkSpace(){ return *state;}
  Player *state = NULL;
  dGeomID ODEgeomID;
  Ogre::SceneNode *OgreNode;
  Ogre::Entity *OgreEntity;
};



class Board
{
public:
  Board(int x, int z, int y, double margin);
  //Board(char* meshType);
  ~Board(){delete state;}
  void setPosition(int x, int z, int y, double margin);
  void printBoard();
  bool checkBoard();
  bool updateState(Player *p);
  Space *spaces[9];
  Player *state = NULL;
  dGeomID ODEgeomID;
  dSpaceID ODEspaceID;
  Ogre::SceneNode *OgreNode;
  Ogre::Entity *OgreEntity;
  int posX, posY, posZ;
};


//template <class T>
class MainBoard  //inherits from board
{
  public:
    MainBoard();
    ~MainBoard(){}
    void printBoard();
    bool makeMove(Player *player, unsigned int j, unsigned int i);
    bool checkBoard();


    Player *state = NULL;
    dGeomID ODEgeomID;
    Ogre::SceneNode *OgreNode;
    Ogre::Entity *OgreEntity;
    //try and make spaces generic?
    Board *spaces[9];

};
