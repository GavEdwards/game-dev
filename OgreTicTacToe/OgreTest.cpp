
//local files
#include "t2.h"

//game variables
MainBoard* mb;

Player *p1;
Player *p2;

Player *turn;
unsigned int boardNo, spaceNo;
unsigned int lastMove = 20;

// OGRE variables
Ogre::Root *root;
Ogre::RenderWindow *window;
Ogre::Viewport *vp;
//smgr is also refernced in the other file using extern
Ogre::SceneManager *smgr;



Ogre::SceneNode *o_ground;
Ogre::SceneNode *buggyNode, *wheel1node, *wheel2node, *wheel3node, *wheel4node;

Ogre::SceneNode *bullets[5];
Ogre::SceneNode *spacesNode[9];

// Camera
Ogre::Camera *cam;
Ogre::Camera *camFP;
Ogre::SceneNode *FPcamNode;

bool firstPerson = false;
float camx = 0.0, camy = 40.75, camz = 0.0;
float camradius = 2.0, camangle = 0.0;

float camxFP = 2.0, camyFP = 2, camzFP = 0.0;
float camradiusFP = 2.0, camangleFP = 0.0;
float camRotXFP = 6.0, camRotZFP = 0.0;
OIS::Mouse *mouse;
size_t windowHnd = 0;

//Overlay UI
Ogre::Overlay *overlay;
Ogre::OverlayElement *mainPanel;
Ogre::TextAreaOverlayElement* infoText;
Ogre::TextAreaOverlayElement* infoText2;
Ogre::TextAreaOverlayElement* gameState;
Ogre::OverlayManager* overlayMgr;

//user input manager (show mouse etc)
OIS::InputManager* im;

// Function prototypes (that are defined later)
void initialise();
static void simLoop(int pause);
void fireBullet();
bool cannonMode();
bool destroyTargets();

// some constants
#define LENGTH 0.7	// chassis length
#define WIDTH 0.5	// chassis width
#define HEIGHT 0.2	// chassis height
#define RADIUS 0.18	// wheel radius
#define STARTZ 0.5	// starting height of chassis
#define CMASS 0.5		// chassis mass
#define WMASS 0.2	// wheel mass
#define BMASS 0.01		// bullet mass

// dynamics and collision objects (chassis, 3 wheels, environment)

static dWorldID world;
//dworld space refernces by other file using extern
dSpaceID space;

static dBodyID body[6]; //previously 5
static dJointGroupID contactgroup;
static dGeomID ground;
static dGeomID spaces[9];

// things that the user controls

static dReal speed=0,steer=0;	// user commands


double CANNON_X = 0;		// x position of cannon
double CANNON_Z = 0.6;	// x position of cannon
double CANNON_Y = 0;		// y position of cannon
#define CANNON_BALL_MASS 10	// mass of the cannon ball
#define CANNON_BALL_RADIUS 0.25
static const dVector3 xunit = { 1, 0, 0 }, yunit = { 0, 1, 0 }, zpunit = { 0, 0, 1 }, zmunit = { 0, 0, -1 };
static dGeomID cannon_ball_geom;
static dBodyID cannon_ball_body;
static Ogre::SceneNode *cannonBallNode;
static Ogre::Entity *cannonBall;
static Ogre::SceneNode *cannonNode;
static Ogre::Entity *cannonBox;
static Ogre::SceneNode *cannonBarrelNode;
static Ogre::Entity *cannonBarrel;

static dGeomID target_geom[3];
static dBodyID target_body[3];
static Ogre::SceneNode *targetNode[3];
static Ogre::Entity *target[3];

static bool targetsHit = false;
static int cannonPosX = 0;
static int cannonPosY = 0;

static bool t1 = false;
static bool t2 = false;
static bool t3 = false;

static dReal cannon_angle=-0.04,cannon_elevation= -1.55;

bool destroyTargets()
{
  cannonNode->setVisible(false);

  for(int i = 0; i<3; i++){
    dBodyDestroy(target_body[i]);
    smgr->destroySceneNode(targetNode[i]);
  }
}
bool cannonMode()
{
  infoText2->setCaption("CANNON MODE - shoot the enemies to claim that space!");
  cannonNode->setVisible(true);

  dMass m;


  for(int i = 0; i<3; i++){

    target_body[i] = dBodyCreate(world);
    dBodySetPosition(target_body[i], std::rand()%20-10+(cannonPosX),std::rand()%2+1,std::rand()%20-10+(cannonPosY));
    //dBodySetPosition(target_body[i], 5,2,5);

    dMassSetBox (&m,1,5,2,5);
    dMassAdjust (&m,CMASS);
    dBodySetMass (target_body[0],&m);

    target_geom[i] = dCreateBox(space,1,1,1);
    dGeomSetBody(target_geom[i], target_body[i]);

    targetNode[i] = smgr->getRootSceneNode()->createChildSceneNode();
    target[i] = smgr->createEntity("robot.mesh");
    //target[i]->setMaterialName("CSC-30019/PlainGreen");
    targetNode[i]->attachObject(target[i]);

    const dReal *boxpos = dBodyGetPosition(target_body[i]);
    const dReal *boxrot = dBodyGetQuaternion(target_body[i]);

    targetNode[i]->setPosition(boxpos[0], boxpos[1], boxpos[2]);
    targetNode[i]->setOrientation(boxrot[0], boxrot[1], boxrot[2], boxrot[3]);
    targetNode[i]->scale(0.02,0.02,0.02);

  }

  firstPerson = true;

}

void fireBullet()
{

  Ogre::Quaternion rotation = cannonNode->getOrientation();
  Ogre::Quaternion rotation2 = cannonBarrelNode->getOrientation();

  dMatrix3 R2,R3,R4;
  dRFromAxisAndAngle (R2,0,0,1,cannon_angle);
  dRFromAxisAndAngle (R3,0,1,0,cannon_elevation);
  dMultiply0 (R4,R2,R3,3,3,3);
  dReal cpos[3] = {cannonPosX,CANNON_Z,cannonPosY};

  dBodySetPosition (cannon_ball_body,cpos[0],cpos[1],cpos[2]);
  dReal force = 8;

  dBodySetLinearVel (cannon_ball_body,force*R4[2],force*(-rotation2[3]),force*R4[10]);
  dBodySetAngularVel (cannon_ball_body,0,0,0);

}

// this is called by dSpaceCollide when two objects in space are
// potentially colliding.
static void nearCallback (void *data, dGeomID o1, dGeomID o2)
{
  int i,n;

  // only collide things with the ground and targets
  int g1 = (o1 == ground || o1 == target_geom[0] || o1 == target_geom[1] || o1 == target_geom[2]);
  int g2 = (o2 == ground || o2 == target_geom[0] || o2 == target_geom[1] || o2 == target_geom[2]);
  if (!(g1 ^ g2)) return;

  /*

  NEEDS WORK!!!!!
  */
  if(firstPerson){

  if(o2 == target_geom[0] && o1 == cannon_ball_geom)
  {
    t1=true;
  }
  if(o2 == target_geom[1] && o1 == cannon_ball_geom)  {
      t2=true;
    }
  if(o2 == target_geom[2] && o1 == cannon_ball_geom)   {
      t3=true;
    }

}

  const int N = 4;
  dContact contact[N];
  n = dCollide (o1,o2,N,&contact[0].geom,sizeof(dContact));
  if (n > 0) {
      for (i=0; i<n; i++) {
          contact[i].surface.mode = dContactSlip1 | dContactSlip2 |
          dContactSoftERP | dContactSoftCFM | dContactApprox1;
          contact[i].surface.mu = dInfinity;
          contact[i].surface.slip1 = 0.1;
          contact[i].surface.slip2 = 0.1;
          contact[i].surface.soft_erp = 0.5;
          contact[i].surface.soft_cfm = 0.3;
          dJointID c = dJointCreateContact (world,contactgroup,&contact[i]);
          dJointAttach (c,
                        dGeomGetBody(contact[i].geom.g1),
                        dGeomGetBody(contact[i].geom.g2));
      }
  }

}


// simulation loop
static void gameLoop() {

    // Initialise OIS using parameters that leave the mouse pointer visible: (n.b. different for Windows/Mac)
    OIS::ParamList paramList;
    std::ostringstream windowHndStr;
    windowHndStr << windowHnd;
    paramList.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));
    paramList.insert(std::make_pair(std::string("x11_mouse_grab"), std::string("false")));
    paramList.insert(std::make_pair(std::string("x11_mouse_hide"), std::string("false")));
    paramList.insert(std::make_pair(std::string("x11_keyboard_grab"), std::string("false")));
    paramList.insert(std::make_pair(std::string("XAutoRepeatOn"), std::string("true")));

    im = OIS::InputManager::createInputSystem(paramList);


    // Overlay data
    mainPanel = static_cast<Ogre::OverlayElement*> (overlayMgr->getOverlayElement("CSC-30019/Example3Overlay/MainPanel"));
    Ogre::TextAreaOverlayElement* title = static_cast<Ogre::TextAreaOverlayElement*> (overlayMgr->getOverlayElement("CSC-30019/Example3Overlay/MainPanel/Title"));
    title->setCaption("CSC-30019 TicTacToe UNIVERSE");

    infoText = static_cast<Ogre::TextAreaOverlayElement*> (overlayMgr->getOverlayElement("CSC-30019/Example3Overlay/MainPanel/InfoText"));
    infoText->setCaption("Turn:");

    infoText2 = static_cast<Ogre::TextAreaOverlayElement*> (overlayMgr->getOverlayElement("CSC-30019/Example3Overlay/MainPanel/InfoText2"));
    infoText2->setCaption("Make your first move by clicking on any space");

    gameState = static_cast<Ogre::TextAreaOverlayElement*> (overlayMgr->getOverlayElement("CSC-30019/Example3Overlay/MainPanel/GameState"));
    gameState->setCaption("Game State");

    // The actual overlay
    overlay = overlayMgr->getByName("CSC-30019/Example3Overlay");
    overlay->show();


    //set keyboard input and mouse input
    OIS::Keyboard* keyboard = static_cast<OIS::Keyboard*>(im->createInputObject(OIS::OISKeyboard, true));
    mouse = static_cast<OIS::Mouse*>(im->createInputObject(OIS::OISMouse, false));

    // We will use Ogre's hi-res timer to control the game loop
    Ogre::Timer *timer = new Ogre::Timer();

    // Variables for the game loop - see lecture 2
    long currentTime=timer->getMilliseconds(), lastTime=currentTime;
    long frame_length = 20; // milliseconds
    long accumulator = 0;
    int maxUpdatesPerFrame = 10;


    //ray tracing
    Ogre::RaySceneQuery* mSceneQuery;
    mSceneQuery = smgr->createRayQuery(Ogre::Ray());
    mSceneQuery->setSortByDistance(true);
    bool gameFinished = false;
    while( true ) // or until we use the break statement...
    {

        int updatesMade = 0;

        // Required to allow Ogre to pass messages (like Window Closed) to the OS
        Ogre::WindowEventUtilities::messagePump();


        // Collect some input
        keyboard->capture();
        mouse->capture();
        const OIS::MouseState state = mouse->getMouseState();

        if (keyboard->isKeyDown(OIS::KC_ESCAPE)) break;
        if( keyboard->isKeyDown(OIS::KC_A)) { speed += 0.13; }
        if( keyboard->isKeyDown(OIS::KC_Z)) { speed -= 0.13;}
        if( keyboard->isKeyDown(OIS::KC_COMMA)) steer -= 0.05;
        if( keyboard->isKeyDown(OIS::KC_PERIOD)) steer += 0.05;
        //if( keyboard->isKeyDown(OIS::KC_SPACE)) { speed = 0; steer = 0; }
        //if( keyboard->isKeyDown(OIS::KC_F)) { firstPerson = true;}
        if( keyboard->isKeyDown(OIS::KC_T)) { camx = 0.0; camy = 40.75; camz = 0.0; camradius = 2.0; camangle = 0.0;}

        if(firstPerson){

          Ogre::Quaternion rotation = cannonNode->getOrientation();
          Ogre::Quaternion rotation2 = cannonBarrelNode->getOrientation();

          //std::cout<< rotation << std::endl;
          if( keyboard->isKeyDown(OIS::KC_UP)){
            cannonBarrelNode->roll(Ogre::Degree(4));
            cannon_angle=rotation2[3];
          }
          if( keyboard->isKeyDown(OIS::KC_DOWN)){
            cannonBarrelNode->roll(Ogre::Degree(-4));
            cannon_angle=rotation2[3];
          }
          if( keyboard->isKeyDown(OIS::KC_LEFT)){
            cannonNode->yaw(Ogre::Degree(10));
            cannon_elevation+= 0.175;//-1.5 + rotation[0]*2;
          }
          if( keyboard->isKeyDown(OIS::KC_RIGHT)){
            cannonNode->yaw(Ogre::Degree(-10));
            cannon_elevation-= 0.175;//-1.5 + rotation[0]*2;
          }
          if( keyboard->isKeyDown(OIS::KC_SPACE)) fireBullet();


        } else {
          if( keyboard->isKeyDown(OIS::KC_UP)) camradius += 0.5;
          if( keyboard->isKeyDown(OIS::KC_DOWN)) camradius -= 0.5;
          if( keyboard->isKeyDown(OIS::KC_LEFT)) camangle -= M_PI / 30.0;
          if( keyboard->isKeyDown(OIS::KC_RIGHT)) camangle += M_PI / 30.0;
        }

        // Work out how much time has elapsed, add to accumulator
        lastTime = currentTime;
        currentTime=timer->getMilliseconds();
        long elapsedTime = currentTime - lastTime;
        accumulator += elapsedTime;

        // If we're on a fast PC we can "sleep" allowing other programs time on the CPU
        if( accumulator < frame_length ) {
            long frameRemainingMicroSec = 1000*(frame_length-accumulator);
            usleep(frameRemainingMicroSec);
        }

        while( accumulator >= frame_length && updatesMade++ < maxUpdatesPerFrame ) {

            // do game logic, physics etc;
            simLoop(0);
            accumulator -= frame_length;
        }

        if(firstPerson){

          //camFP->lookAt(buggyNode->getPosition());
          camFP->lookAt(cannonNode->getPosition());

          //camFP->lookAt(Ogre::Vector3(camxFP,0,0));
          //FPcamNode->setOrientation(camRotXFP, .0 , camRotZFP, .0);
          //FPcamNode->setPosition(camRotXFP, .0 , camRotZFP);
          vp->setCamera(camFP);

        } else {
          // Camera is stuck on a circle on the X-Z plane, camradius from the origin at camangle
          cam->setPosition(Ogre::Vector3(camx,camy,camz));
          camx = cosf(camangle) * camradius;
          camz = sinf(camangle) * camradius;
          cam->lookAt( Ogre::Vector3(0,0,0) );
          vp->setCamera(cam);

        }

        if(!gameFinished){

          if(firstPerson)
          {
            /*

            NEEDS WORK!!!!!
            */
            static int countz = 0;

            if(t1 && t2 && t3){
              std::cout<<"COMPLETED"<<std::endl;
              infoText2->setCaption("CONGRATULATIONS you defeated the enemies!");
              destroyTargets();
              t1 = false;
              t2=false;
              t3=false;
              countz = 0;
              firstPerson = false;
            } else{

              countz++;

              if(countz >(300) ){
                infoText2->setCaption("OH DEAR! you ran out of time and lost that space!");
                std::cout<< "END CANNON MODE" << std::endl;
                destroyTargets();
                t1 = false;
                t2=false;
                t3=false;
                countz = 0;
                mb->spaces[boardNo]->spaces[spaceNo]->state = NULL;
                mb->spaces[boardNo]->state == NULL;
                mb->spaces[boardNo]->spaces[spaceNo]->OgreEntity->setMaterialName( "CSC-30019/Tile1");//"CSC-30019/Tile1-Selected");
                mb->spaces[boardNo]->OgreEntity->setMaterialName( "CSC-30019/Galaxy");//"CSC-30019/Tile1-Selected");

                firstPerson = false;
            }
          }


          } else {

        /*
        *  Ray tracing code for mouse pointer
        */
        //if the mouse is clicked
            static bool mouseReleased = true;
            if( !mouse->getMouseState().buttonDown(OIS::MB_Left) ){
              mouseReleased = true;
            }

            if( mouseReleased && mouse->getMouseState().buttonDown(OIS::MB_Left) ){

              mouseReleased = false;
               std::cout<<"MOUSE CLICK\n";

              // Get normalised mouse coordinates
              float sc_x = (float) state.X.abs / vp->getActualWidth();
              float sc_y = (float) state.Y.abs / vp->getActualHeight();

              // Create a ray using the mouse coordinates
              Ogre::Ray pick_ray = cam->getCameraToViewportRay(sc_x, sc_y);

              // Give the scene query the ray
              mSceneQuery->setRay(pick_ray);

              // Execute the scene query
              Ogre::RaySceneQueryResult& result = mSceneQuery->execute();

              // If we had a hit in the ray query, get the entity from the first result
              // (there may be more than one) and set it to a "selected" material:
              if( !result.empty() )
              {
                mainPanel->setMaterialName("CSC-30019/PlainBlue");
                if( state.X.abs >= 10 && state.X.abs < 510 && state.Y.abs >=10 && state.Y.abs < 80 ){

                    mainPanel->setMaterialName("CSC-30019/PlainGreen");

                  }
                  else {


                    for(int j = 0; j<9; j++){
                      for(int i = 0; i<9; i++){
                        if((result[0].movable)->getParentSceneNode() == mb->spaces[j]->spaces[i]->OgreNode) //->movable->getName().compare("Ogre/MO1") == 0)
                        {
                          mb->spaces[j]->spaces[i]->OgreNode->showBoundingBox(true);

                          std::cout<< "It is " << turn->getName() <<"'s turn!" << std::endl;

                          if( mb->makeMove(turn,j,i, lastMove) )
                          {
                            /*

                            NEEDS WORK!!!!!
                            */
                            mb->spaces[spaceNo]->OgreNode->showBoundingBox(false);
                            boardNo = j;
                            spaceNo = i;
                            mb->spaces[spaceNo]->OgreNode->showBoundingBox(true);

                            int randomV = std::rand()%10;
                            std::cout<< randomV << std::endl;
                            if(randomV < 3){
                              const dReal *boardPos = dGeomGetPosition(mb->spaces[j]->spaces[i]->ODEgeomID);
                              cannonPosX = boardPos[0];
                              cannonPosY = boardPos[1];
                              boardNo = j;
                              spaceNo = i;
                              cannonMode();
                            }

                            //change turn for the next round
                            if(turn == p1){

                            mb->spaces[j]->spaces[i]->OgreEntity->setMaterialName( "CSC-30019/Space1");//"CSC-30019/Tile1-Selected");
                            //mb->spaces[j]->spaces[i]->OgreNode->attatchObject(smgr->createLight() );
                              if( mb->spaces[j]->checkBoard() )
                              {

                                  mb->spaces[j]->OgreEntity->setMaterialName( "CSC-30019/GalaxyGreen");
                                  if( mb->checkBoard() ){
                                      mainPanel->setMaterialName("CSC-30019/PlainGreen");
                                     gameFinished = true; break;
                                   }
                              }

                              turn = p2;
                              mainPanel->setMaterialName("CSC-30019/PlainBlue");

                            }
                            else if(turn == p2){
                            mb->spaces[j]->spaces[i]->OgreEntity->setMaterialName( "CSC-30019/Space2");//"CSC-30019/Tile1-Selected");
                          //mb->spaces[j]->spaces[i]->OgreNode->attatchObject(smgr->createLight() );

                              if( mb->spaces[j]->checkBoard() )
                              {
                                  mb->spaces[j]->OgreEntity->setMaterialName( "CSC-30019/GalaxyBlue");
                                  if( mb->checkBoard() ){
                                      mainPanel->setMaterialName("CSC-30019/PlainBlue");
                                     gameFinished = true; break;
                                   }
                              }
                              turn = p1;
                              mainPanel->setMaterialName("CSC-30019/PlainGreen");

                            }

                            lastMove = i;

                          }

                          mb->printBoard();

                        } else {
                          mb->spaces[j]->spaces[i]->OgreNode->showBoundingBox(false);

                        }//end if a board space object
                      }//end for loop for space
                    }//end for loop for board
                  }//end if overlay clicked
              }//end if not empty

              std::cout<< "It is " << turn->getName() <<"'s turn!" << std::endl;
              infoText->setCaption("It is " + std::string( turn->getName() ) + "'s turn!");

            }//end if click
      }
}//end if game not finished loop
        // renderOneFrame returns false if it fails, so we will bail in that case.
        if(root->renderOneFrame() == false)
            break;

    }
}

static void setPlayers(char *name1, char *name2)
{
  p1 = new Player(name1);
  p2 = new Player(name2);
  std::cout<< "Created Player 1: " << p1->getName() << std::endl;
  std::cout<< "Created Player 2: " << p2->getName() << std::endl;
  //make player 1 have their turn first
  turn = p1;

}


// This is the old DrawStuff simLoop - hacked to do Ogre stuff instead!
// (And now it's called from our game loop, rather than DrawStuff's)
static void simLoop (int pause)
{
    int i;

    if (!pause) {

        dSpaceCollide (space,0,&nearCallback);
        dWorldStep (world,0.05);

        // remove all contact joints
        dJointGroupEmpty(contactgroup);
    }

    // GET POSITIONS OF EVERYTHING FROM PHYSICS, UPDATE GRAPHICS

    // draw the cannon
    Ogre::Quaternion rotation = cannonNode->getOrientation();

    Ogre::Quaternion rotation2 = cannonBarrelNode->getOrientation();

  	dReal cpos[3] = {cannonPosX,CANNON_Z,cannonPosY};

    cannonNode->setPosition(cpos[0], cpos[1], cpos[2]);
    cannonBarrelNode->setPosition(cpos[0], cpos[1]+50, cpos[2]);
  	//draw the cannon ball
    const dReal *Ballpos = dBodyGetPosition(cannon_ball_body);
    //const dReal *Ballrot = dBodyGetQuaternion(cannon_ball_body);

    cannonBallNode->setPosition(Ballpos[0], Ballpos[1], Ballpos[2]);
    //cannonBallNode->setOrientation(Ballrot[0], Ballrot[1], Ballrot[2], Ballrot[3] );

    if(firstPerson){
      for(int h =0; h<3; h++){
      const dReal *Tpos = dBodyGetPosition(target_body[h]);
      const dReal *Trot = dBodyGetQuaternion(target_body[h]);

      targetNode[h]->setPosition(Tpos[0], Tpos[1]-1, Tpos[2]);
      targetNode[h]->setOrientation(Trot[0], Trot[1], Trot[2], Trot[3]);
      }
    }

}


int main (int argc, char **argv)
{
    int i;
    dMass m;

    // OGRE Initialisation
    initialise();

    // BEGIN old ODE code which does the buggy simulation
    // create world
    dInitODE2(0);
    world = dWorldCreate();
    space = dHashSpaceCreate (0);
    contactgroup = dJointGroupCreate (0);
    dWorldSetGravity (world,0,-0.5,0);
    ground = dCreatePlane (space,0,1,0,0);

    //create cannon
    cannon_ball_body = dBodyCreate (world);
  	cannon_ball_geom = dCreateSphere (space,CANNON_BALL_RADIUS);
  	dMassSetSphereTotal (&m,CANNON_BALL_MASS,CANNON_BALL_RADIUS);
  	dBodySetMass (cannon_ball_body,&m);
  	dGeomSetBody (cannon_ball_geom,cannon_ball_body);
  	dBodySetPosition (cannon_ball_body,CANNON_X,CANNON_Z,CANNON_Y);

    // environment
//    dMatrix3 R;
  //  dRFromAxisAndAngle (R,0,0,1,-0.15);
    //dGeomSetRotation (ground_box,R);


    //create the board
    //game initialise
    mb = new MainBoard();


    // Set the scene
    window->getCustomAttribute("WINDOW", &windowHnd);
    cam = smgr->createCamera("Camera1");
    camx = cosf(camangle) * camradius; camz = sinf(camangle) * camradius;
    cam->setPosition(Ogre::Vector3(camx,camy,camz));
    cam->lookAt(Ogre::Vector3(0,0,0));
    cam->setNearClipDistance(1);
    vp = window->addViewport(cam); //use for ray tracing from camera view
    cam->setAspectRatio(Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()) );


    // SKYBOX
    smgr->setSkyBox(true, "CSC-30019/SpaceSkyBox");



    //cannon
    cannonNode = smgr->getRootSceneNode()->createChildSceneNode();
    cannonBox = smgr->createEntity("cube.mesh");
    cannonNode->attachObject(cannonBox);
    cannonNode->scale(0.002,0.002,0.002);

    cannonBarrelNode = cannonNode->createChildSceneNode();
    cannonBarrel = smgr->createEntity("cube.mesh");
    cannonBarrelNode->scale(2,0.2,0.2);
    cannonBarrelNode->attachObject(cannonBarrel);
    //cannonBarrelNode->scale(0.01,0.01,0.01);

    cannonNode->setVisible(false);

    cannonBallNode = smgr->getRootSceneNode()->createChildSceneNode();
    cannonBall = smgr->createEntity("sphere.mesh");
    cannonBallNode->attachObject(cannonBall);
    cannonBallNode->scale(0.001,0.001,0.001);


    //cannon camera

    camFP = smgr->createCamera("CameraFP");
    camFP->setNearClipDistance(0.2);
    camFP->setAspectRatio(Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()) );


    FPcamNode = cannonNode->createChildSceneNode("FPcamNode");
    FPcamNode->scale(0.0001,0.0001,0.0001);
    FPcamNode->setPosition(400,100,0);
    FPcamNode->attachObject(camFP);

    // Set the ambient lighting in the scene
    smgr->setAmbientLight(Ogre::ColourValue(0.3,0.3,0.4));

    // Directional lighting so we get nice shadows
    Ogre:: Light *dirLight = smgr->createLight("dir_light");
    dirLight->setType(Ogre::Light::LT_DIRECTIONAL );
    dirLight->setDirection(Ogre::Vector3(0.707,-.707,0));
    dirLight->setDiffuseColour(0.8,0.8,0.8);

    // Shadows have to be enabled though
    smgr->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_ADDITIVE);


    //WRAP IN TRY CATCH TO VALIDATE USER INPUT
    /*
      char n1[15], n2[15];
      std::cout<<"Enter player 1 name: ";
      std::cin >> n1;
      std::cout<<"Enter player 2 name: ";
      std::cin>> n2;

      setPlayers(n1, n2);
      */
    setPlayers("player1", "player2");

    // render a frame for good measure
    root->renderOneFrame();
    // END OGRE Initialisation

    // run simulation (using our own game loop)
    gameLoop();

    // tidy up
    dGeomDestroy (ground);
    dGeomDestroy(cannon_ball_geom);

    //for(int x =0; x<9;x++)
    //dGeomDestroy (mb->spaces[0]->spaces[x]->ODEgeomID);

    dJointGroupDestroy (contactgroup);
    dSpaceDestroy (space);
    dWorldDestroy (world);
    dCloseODE();
    return 0;
}



void initialise()
{

    std::string resourcePath;
    resourcePath = "";

    root = new Ogre::Root(resourcePath + "plugins.cfg", resourcePath + "ogre.cfg", "Ogre.log");

    if (!root->showConfigDialog())
        exit(-1);

    Ogre::ConfigFile cf;
    cf.load(resourcePath + "resources.cfg");

    // Go through all sections & settings in the file
    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

    Ogre::String secName, typeName, archName;
    while (seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = i->second;

            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
        }
    }


    window = root->initialise(true);

    smgr = root->createSceneManager(Ogre::ST_GENERIC);

    // The basic Overlay system need to be initialised *before* resource groups!
    Ogre::OverlaySystem * overlaySystem = new Ogre::OverlaySystem;
    overlayMgr = Ogre::OverlayManager::getSingletonPtr();

    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

    // Add the overlay system to the scene manager's render queue
    smgr->addRenderQueueListener( overlaySystem );


}
