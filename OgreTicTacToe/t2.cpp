#include "t2.h"

//extern as variable is located in another file
extern Ogre::SceneManager *smgr;
extern dSpaceID space;



Board::Board(int x, int y, int z, double margin)
{

  //dGeomSetRotation (spaces[i],R2);
  ODEspaceID = dSimpleSpaceCreate(space);
  dSpaceSetCleanup(ODEspaceID, 0);

  ODEgeomID = dCreateBox(ODEspaceID,80,1,80);//

  dGeomSetPosition (ODEgeomID, x ,z, y );

  OgreNode = smgr->getRootSceneNode()->createChildSceneNode();//give it a unique name?
  const dReal *boardPos = dGeomGetPosition(ODEgeomID);
  dReal boardRot[4];  dGeomGetQuaternion(ODEgeomID, boardRot);
  OgreNode->setPosition(boardPos[0],boardPos[1],boardPos[2]);
  OgreNode->setOrientation( boardRot[0], boardRot[1], boardRot[2], boardRot[3]);

  OgreEntity = smgr->createEntity("cube.mesh");//
  OgreEntity->setMaterialName("CSC-30019/Tile2");;
  OgreNode->attachObject(OgreEntity);

  OgreNode->scale(0.1,0.0001,.1);

  int count = 0;
  static double marg = 2;

  for(int i =0; i < 9; i++){

    int x2 = i%3;
    int y2 = count;

    if(x2==2) count++;


    this->spaces[i] =  new Space( x-3+(x2)+(x2)*marg ,0, y-3+(y2)+(y2)*marg,this->ODEspaceID);
    //this->spaces[i] =  new Space(x,y,z, this->ODEspaceID);

  }


}


void Board::setPosition(int x, int z, int y, double margin)
{

  OgreNode->setPosition((x-1)+((x-1)*margin) ,0, (y-1)+((y-1)*margin));
  OgreEntity->setMaterialName("CSC-30019/PlainRed");;

}

bool Board::checkBoard(){

  bool match = false;
  Player *winningPlayer;

    for(int i = 0; i < 7; i += 3){
      //check for horizontal wins
      Player *horizontalTemp = this->spaces[i]->state;
      bool hmatch = true;
      //winningPlayer = horizontalTemp;

      for(int j =0; j<3; j++)
      {

        if(this->spaces[i + j]->state == NULL || this->spaces[i + j]->state != horizontalTemp)
        {
          hmatch = false;
          break;
        } else {
          winningPlayer = this->spaces[i + j]->state;
          //std::cout << winningPlayer << "!!!" << winningPlayer->getName() << "!!!" <<std::endl;
        }

      }

      if(hmatch){
        match = true; break;
      }

      //check for verticle wins

      Player *verticleTemp = this->spaces[i/3]->state;
      bool vmatch = true;

      for(int j =0; j<7; j+=3)
      {

        if(this->spaces[i/3 + j]->state == NULL || this->spaces[i/3 + j]->state != verticleTemp)
        {
          vmatch = false;
          break;
        }else {
          winningPlayer = this->spaces[i/3 + j]->state;
        }

      }

      if(vmatch) match = true;


      }

      if(match){
        std::cout<<"Board Won! by: " << winningPlayer->getName();
        //std::cout << " | "<< winningPlayer->getName() << std::endl;
        //std::cout << "\n before: " << this->Board::state<<std::endl;
        this->Board::state = winningPlayer;
        //this->OgreEntity->setMaterialName("CSC-30019/GrassFloor");
        //std::cout << "\n after: " << this->Board::state->getName() <<std::endl;
        return true;
    } else {
      std::cout<<"no board won" <<std::endl;

      return false;
    }
}

void Board::printBoard()
{

    for(int i =0; i < SPACES_PER_BOARD; i++)
    {
      if(this->spaces[i]->state == NULL)
      std::cout << this->spaces[i]->state << " |";
      else
      std::cout << this->spaces[i]->state->getName() <<  " |";

      if((i+1)%3 == 0)
      std::cout<<std::endl;

    }

}


MainBoard::MainBoard()
{
  std::cout<< "START\n" << std::endl;


  std::cout<<"\nboard spaces: "<< sizeof(this->spaces)/sizeof(this->spaces[0]) << std::endl;
  std::cout<<"inner board spaces: "<< sizeof(this->spaces[0]->spaces)/sizeof(this->spaces[0]->spaces[0]) << std::endl<< std::endl;

  ODEgeomID = dCreatePlane(space,0,1,0,0);//

  Ogre::Plane oground(0,1,0,0);
  Ogre::MeshManager::getSingleton().createPlane("ground", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                oground, 35, 35, 1, 1, true, 1, 35, 35, Ogre::Vector3::UNIT_Z );
  Ogre::Entity *OgreEntity = smgr->createEntity("ground");
  OgreEntity->setMaterialName("CSC-30019/GrassFloor");
  smgr->getRootSceneNode()->createChildSceneNode()->attachObject( OgreEntity );

  int count = 0;
  double margin = 11;

  for(int i =0; i < 9; i++){

    int x = i%3;
    int y = count;

    if(x==2) count++;

    spaces[i] =  new Board( (x-1)+((x-1)*margin) , (y-1)+((y-1)*margin), 0, margin);

  }



}

void MainBoard::printBoard()
{

  //std::cout<<"TESTING AGAIN: " << this->spaces[0].Board::state <<std::endl;
  for(int j =0; j< SPACES_PER_BOARD; j++)
  {


    this->spaces[j]->printBoard();


    if( this->spaces[j]->state == NULL)
    {
      std::cout <<"-------------"<< this->spaces[j]->state << std::endl;
      std::cout << "NULL\n";

    } else
    {
      std::cout <<"-------------"<< this->spaces[j]->state->getName() <<  std::endl;
      std::cout << "NOT NULL\n";
    }

    if((j+1)%3 == 0)
      std::cout<<std::endl;
  }

}

bool MainBoard::makeMove(Player *player, unsigned int j, unsigned int i)
{

  if( j < 9 && i <9 && this->spaces[j]->spaces[i]->state == NULL && this->spaces[j]->state ==NULL )
  {
    this->spaces[j]->spaces[i]->state = player;
    return true;
  }
  else
  {
    std::cout<<"ILLEGAL MOVE"<<std::endl;
    return false;
  }
}


bool MainBoard::checkBoard()
{

  bool match = false;
  Player *winningPlayer;

  std::cout<< "CHECKING FOR MAIN WIN\n";
    for(int i = 0; i < 7; i += 3){

      bool hmatch = true;

      //check for horizontal wins
      Player *horizontalTemp = this->spaces[i]->state;
      winningPlayer = horizontalTemp;

      for(int j =0; j<3; j++)
      {

        if(this->spaces[i + j]->state == NULL || this->spaces[i + j]->state != horizontalTemp)
        {
          hmatch = false;
          break;
        }else {
          winningPlayer = this->spaces[i + j]->state;
        }

      }

      if(hmatch){
         match = true; break;
       }
      //check for verticle wins
      Player *verticleTemp = this->spaces[i/3]->state;
      bool vmatch = true;

      for(int j =0; j<7; j+=3)
      {

        if(this->spaces[i/3 + j]->state == NULL || this->spaces[i/3 + j]->state != verticleTemp)
        {
          vmatch = false;
          break;
        } else {
          winningPlayer = this->spaces[i/3 + j]->state;
        }
      }

      if(vmatch) match = true;


      }

      std::cout<<"CHECKS DONE"<< std::endl;

      if(match){
        /*

        * THIS NEEDS FIXING TO PRINT PLAYER NAME WITHOUT A SEG FAULT

        */

        std::cout<< "---------------\n"<<"GAME WON! by: " << winningPlayer->getName();
        std::cout << "\n---------------"<<std::endl;

        this->state = winningPlayer;

        return true;
    } else {
      std::cout << "----------------\n" << "No Win.. Game On!" << "\n-----------------" <<std::endl;
      return false;
    }
}




Space::Space(int x, int z, int y, dSpaceID space)
{

  ODEgeomID = dCreateBox(space,1,1,1);//


  dGeomSetPosition (ODEgeomID, x , z , y );
  //dGeomSetRotation (spaces[i],R2);

  OgreNode = smgr->getRootSceneNode()->createChildSceneNode();//give it a unique name?
  const dReal *boardPos = dGeomGetPosition(ODEgeomID);
  dReal boardRot[4];  dGeomGetQuaternion(ODEgeomID, boardRot);
  OgreNode->setPosition(boardPos[0],boardPos[1],boardPos[2]);
  OgreNode->setOrientation( boardRot[0], boardRot[1], boardRot[2], boardRot[3]);

  OgreEntity = smgr->createEntity("cube.mesh");//
  //OgreEntity = smgr->createEntity("sphere.mesh");
  OgreEntity->setMaterialName("CSC-30019/Tile1");;
  OgreNode->attachObject(OgreEntity);

  OgreNode->scale(.01,.01,.01);


}

void Space::setPosition(int x, int z, int y, double margin)
{

  OgreNode->setPosition((x-1)+((x-1)*margin) ,0, (y-1)+((y-1)*margin));

  //OgreNode->setPosition(boardPos[0],boardPos[1],boardPos[2]);
  //OgreNode->setOrientation( boardRot[0], boardRot[1], boardRot[2], boardRot[3]);

}
