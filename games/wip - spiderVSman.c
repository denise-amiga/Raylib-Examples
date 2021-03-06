//
// STILL A WORK IN PROGRESS
//
// Added - walking animation for player.
// Added - spider turns towards and move to target(press mouse on screen to see this..)
// Added - spiders! can roam and avoid the walls and stop if player gets near them.
// Added - spiders will sprint towards player(attack!) when nearby.
// Added - Eggsacks - spiders will hatch from spidereggs.
// Added - GFX filter on the images on creation/ see processGfx true/false

enum flag1{IDLE,QUICKDASH,SCOUTNEWPOSITION,SEEATTACKPLAYER};
enum flag2{FINDSPOT,SPIDERTURN,FOUNDSPOT};
enum flag3{EGGSACKFULL,EGGSACKEMPTY};

#define fullScreenMode false
#define processGfx true // a filter is aplied to the gfx at creation. disable for the regular pixel art.


#define MAX_TILES 120
#define MAX_SPIDERS 20
#define MAX_EGGSACKS 15 //currently eggsacks needs to be 1 less than max spiders!

#include "raylib.h"
#include <math.h>


typedef struct tileset{
    int frame;
    RenderTexture2D tile;
}tileset;

static struct tileset arr_tileset[MAX_TILES];

typedef struct eggsack{
    Vector2 position;
    bool active;
    int state;
    int width;
    int height;
}eggsack;

static struct eggsack arr_eggsack[MAX_EGGSACKS];

typedef struct spider{
    bool active;
    Vector2 position;
    int frame;
    int time;
    float width;
    float height; 
    float turndirection;
                // state 0 IDLE = turn around(idle)
    int state;  // state 99 QUICKDASH = Quick turn to target and sprint towards target
                // state 199 FINDSPOT = check ahead and see if new destination can be found (roam slowly)
    int substate;
    Vector2 target;
    float realangle;
    float disttarget;
    float angle;
}spider;


typedef struct player{
    Vector2 position;
    float width;
    float height;  
    RenderTexture2D frame[10];
    bool idle;
    int hdirection; // horizontal direction -1 = left - 1 = right
    int frameposition;
    int framewalkstart;//walking is 1 and 2
    int framewalkend;//
    int framewalkdelay;
    int framewalkcount;
}player;


// This is our tile map.
static int tilemap[10][20] = {0};

static int map[10][20] = {  {1,1,1,1,1,1,1,1,1,1,1,1,9,9,1,1,1,9,9,9},
                            {1,0,0,0,0,0,0,0,0,0,0,1,9,9,1,0,1,9,9,9},
                            {1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,9,9,9},
                            {1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,9,9,9},
                            {9,9,9,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
                            {9,9,9,1,0,0,1,1,1,0,0,1,1,1,0,0,0,0,0,1},
                            {1,1,1,1,0,0,1,9,1,0,0,1,9,1,0,0,0,0,0,1},
                            {1,0,0,0,0,0,1,9,1,0,0,1,9,1,1,0,0,0,0,1},
                            {1,0,0,0,0,0,1,9,1,0,0,1,9,9,1,0,0,0,0,1},
                            {1,1,1,1,1,1,1,9,1,1,1,1,9,9,1,1,1,1,1,1}};   

static float tileWidth;
static float tileHeight;
static int mapWidth = 20;//width and height of map dimensions
static int mapHeight = 10;

static player myplayer = {0};
static struct spider myspider[MAX_SPIDERS];


static	Color db32color[32];// ' our colors	

static RenderTexture2D spritespider1; 
static RenderTexture2D spritespider2; 
static RenderTexture2D spriteplayer; 
static RenderTexture2D spriteeggsackfull; 
static RenderTexture2D spriteeggsackempty; 


static void inidb32colors(void);		
static void inisprites(void);
static bool playertilecollide(int offsetx,int offsety);
static bool rectsoverlap(int x1,int y1,int w1,int h1,int x2,int y2,int w2,int h2);
static void maketilemap(void);
static float getangle(float x1,float y1,float x2,float y2);
// Return on which side of the line the point is. l-1 0= r=1
// lineback x,y linefront x,y point x,y
int orientation(int ax,int ay,int bx, int by, int cx, int cy);
static float angledifference(float angle1, float angle2);
static bool spidertilecollide(int index, int offsetx,int offsety);
static bool recttilecollide(int x,int y,int w, int h);
static float getdistance(float x1,float y1,float x2,float y2);
static void DrawRectangle2(int x,int y,int w,int h,Color col);

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;
    
    

    InitWindow(screenWidth, screenHeight, "raylib example.");
    if(fullScreenMode)ToggleFullscreen();
    mapWidth = 20;
    mapHeight = 10;
    tileWidth = (float)screenWidth/mapWidth;
    tileHeight = (float)screenHeight/mapHeight;
    spritespider1=LoadRenderTexture(32,32); 
    spritespider2=LoadRenderTexture(32,32); 
    spriteplayer=LoadRenderTexture(32,32); 
    spriteeggsackempty=LoadRenderTexture(32,32); 
    spriteeggsackfull=LoadRenderTexture(32,32); 
    myplayer.frame[0] = LoadRenderTexture(32,32); 
    myplayer.frame[1] = LoadRenderTexture(32,32); 
    myplayer.frame[2] = LoadRenderTexture(32,32); 
    
    // tile 
    for(int i=0;i<MAX_TILES;i++){
        arr_tileset[i].frame=i;
        arr_tileset[i].tile = LoadRenderTexture(32,32);
    }
    inidb32colors(); 
    inisprites();

    maketilemap();
    
    // add eggsacks
    int cnt=0;
    for(int y=1;y<10;y+=2){
    for(int x=1;x<11;x+=2){
        if(map[y][x]==0){
            if(cnt<MAX_EGGSACKS){                
                arr_eggsack[cnt].active=true;
                arr_eggsack[cnt].position = (Vector2){x*tileWidth+tileWidth/2,y*tileHeight+tileHeight/2};
                arr_eggsack[cnt].width = tileWidth/2;
                arr_eggsack[cnt].height = tileHeight/2;
                arr_eggsack[cnt].state = EGGSACKFULL;
            }
            cnt++;
        }
    }}
    
    // Our player setup
    myplayer.position = (Vector2){352,140};
    myplayer.width = tileWidth/2;
    myplayer.height = tileHeight/2;
    myplayer.hdirection = -1;
    myplayer.frameposition = 1;
    myplayer.framewalkstart = 1;
    myplayer.framewalkend = 2;
    myplayer.framewalkdelay = 5;
    myplayer.idle = true;
    for(int i=0;i<MAX_SPIDERS-MAX_EGGSACKS;i++){
        myspider[i].active=true;
        myspider[i].turndirection=-2;
        if(GetRandomValue(0,10)<5)myspider[i].turndirection=2;
        myspider[i].width = tileWidth/2;
        myspider[i].height = tileHeight/2;
        myspider[i].position = (Vector2){220+tileWidth/2+i*32,180};
        myspider[i].state = 0;
    }
    myspider[0].state = SCOUTNEWPOSITION;
    
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------
    
    int frame=1;
    int time=0;
    float debug=0.0f;
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------

        //
        //
        //eggsack logic        
        bool egghat=false;
        for(int i=0;i<MAX_EGGSACKS;i++){
            if(arr_eggsack[i].active==false)continue;
            if(GetRandomValue(0,4300)==1){                
                if(arr_eggsack[i].state==EGGSACKFULL){
                    arr_eggsack[i].state=EGGSACKEMPTY;
                    for(int j=0;j<MAX_SPIDERS;j++){
                        if(myspider[j].active==false){
                            // first check if there is no spider on this spot.
                            for(int k=0;k<MAX_SPIDERS;k++){
                                if(k==j)continue;
                                if(myspider[k].active==false)continue;                                
                                if(egghat==false){
                                    if(rectsoverlap(arr_eggsack[i].position.x-12,arr_eggsack[i].position.y-12,myspider[0].width+24,myspider[0].height+24,myspider[k].position.x-12,myspider[k].position.y-12,myspider[k].width+24,myspider[k].height+24)==false){
                                        myspider[j].active=true;
                                        myspider[j].turndirection=-2;
                                        if(GetRandomValue(0,10)<5)myspider[j].turndirection=2;
                                        myspider[j].width = tileWidth/2;
                                        myspider[j].height = tileHeight/2;
                                        myspider[j].position = (Vector2){arr_eggsack[i].position.x-8,arr_eggsack[i].position.y};
                                        myspider[j].state = 0;
                                        egghat=true;
                                    }             
                                }                                    
                            }
                            
                        }
                            
                    }
                }
            }
        }




        //spider logic
        
        
        for(int i=0;i<MAX_SPIDERS;i++){                        
            if(myspider[i].active==false)continue;
            
            // Keep spiders apart from each other;
            for(int j=0;j<MAX_SPIDERS;j++){
                if(j==i)continue;
                if(myspider[j].active==false)continue;
                if(rectsoverlap(    myspider[i].position.x-2,myspider[i].position.y-2,myspider[i].width+4,myspider[i].height+4,
                                    myspider[j].position.x-2,myspider[j].position.y-2,myspider[j].width+4,myspider[j].height+4)==true){
                    float pushangle=getangle(myspider[i].position.x,myspider[i].position.y,myspider[j].position.x,myspider[j].position.y);
                    Vector2 oldpos = myspider[i].position;
                    myspider[i].position.x -= cos(pushangle);
                    myspider[i].position.y -= sin(pushangle);
                    if(spidertilecollide(i,0,0)==true){
                        myspider[i].position = oldpos;
                    }
                }
//                if(recttilecollide(myspider[i].position.x-1,myspider[i].position.y-1,myspider[i].width,myspider[i].height-2)){
//                    for(int z=0;z<6;z++){
//                       if(recttilecollide(myspider[i].position.x-1,myspider[i].position.y-1,myspider[i].width,myspider[i].height-2)){
//                        Vector2 oldpos=myspider[i].position;
//                        myspider[i].position.x+=GetRandomValue(-4,4);
//                        myspider[i].position.y+=GetRandomValue(-4,4);
 //                       if(recttilecollide(myspider[i].position.x-1,myspider[i].position.y-1,myspider[i].width,myspider[i].height-2)==true){
//                            myspider[i].position = oldpos;
//                        }
//                        }
//                    }
//                }
            }
            
            // Idle logic
            //
            if(myspider[i].state==IDLE){
                myspider[i].angle+=myspider[i].turndirection;            
                myspider[i].time++;
                if(myspider[i].time>20){
                    myspider[i].frame++;
                    myspider[i].time=0;
                }
                if(GetRandomValue(0,200)==1){
                    if(myspider[i].turndirection=GetRandomValue(-2,2));
                    if(GetRandomValue(0,10)<5)myspider[i].turndirection=0;
                }
                
                if(myspider[i].frame>2)myspider[i].frame=1;
                if(myspider[i].angle>360)myspider[i].angle=0;
                if(myspider[i].angle<0)myspider[i].angle=359;
                
                if(GetRandomValue(0,50)==1){
                    myspider[i].state=SCOUTNEWPOSITION;
                    myspider[i].substate=FINDSPOT;
                }
                if(GetRandomValue(0,20)==1){
                    myspider[i].state=SEEATTACKPLAYER;
                    myspider[i].substate=FINDSPOT;
                }

            }
            //
            //
            //
            // spider SEES player and attacks him
            if(myspider[i].state==SEEATTACKPLAYER){
                myspider[i].time++;
                if(myspider[i].time>2){
                    myspider[i].frame++;
                    myspider[i].time=0;
                }
                if(myspider[i].frame>2)myspider[i].frame=1;
                //
                if(myspider[i].substate==FINDSPOT){
                    bool newspotisgood=true;
                    float x1=myspider[i].position.x;
                    float y1=myspider[i].position.y;
                    
                    myspider[i].target.x = myplayer.position.x+myplayer.width/2;
                    myspider[i].target.y = myplayer.position.y+myplayer.height/2;
                    float angle = getangle(myspider[i].position.x,myspider[i].position.y,myspider[i].target.x,myspider[i].target.y);
                    int distance=getdistance(myspider[i].position.x,myspider[i].position.y,myspider[i].target.x,myspider[i].target.y);    
                    myspider[i].disttarget = distance;
                    if(distance>150){
                        newspotisgood=false;
                        myspider[i].state=IDLE;
                        myspider[i].substate=-1;
                        distance=0;
                    }
                    for(int j=0;j<distance;j++){
                        x1+=cos(angle);
                        y1+=sin(angle);
                        if(recttilecollide(x1,y1,myspider[i].width,myspider[i].height)==true)
                            {
                            newspotisgood=false;
                            myspider[i].substate=-1;
                            myspider[i].state=IDLE;                            
                            j=100;
                        }
                        // if hit other spider than stop idle
                        for(int j=0;j<MAX_SPIDERS;j++){
                            if(i==j)continue;
                            if(rectsoverlap(x1,y1,myspider[i].width,myspider[i].height,myspider[j].position.x,myspider[j].position.y,myspider[j].width,myspider[j].height)==true) {
                                newspotisgood=false;
                                myspider[i].substate=-1;
                                myspider[i].state=IDLE;                            
                                j=100;
                            }
                        }                        
                    }
                    
                    if(newspotisgood){
                        myspider[i].target = (Vector2){x1,y1};
                        myspider[i].substate = SPIDERTURN;
                    }
                }
                if(myspider[i].substate==SPIDERTURN){
                    float angle = getangle(myspider[i].position.x,myspider[i].position.y,myspider[i].target.x,myspider[i].target.y);
                    // turn towards target                
                    float difference = angledifference((myspider[i].angle-90)/180*PI,angle);
                    //debug=difference;
                    if(difference<0)myspider[i].angle-=6;
                    if(difference>0)myspider[i].angle+=6;
                    if(difference>2.9)myspider[i].substate=FOUNDSPOT;
                    if(difference==0)myspider[i].substate=FOUNDSPOT;
                    debug = difference;
                }
                if(myspider[i].substate==FOUNDSPOT){
                    float angle = getangle(myspider[i].position.x,myspider[i].position.y,myspider[i].target.x,myspider[i].target.y);
                    Vector2 oldposition = myspider[i].position;                    
                    myspider[i].position.x += cos(angle)*2;
                    myspider[i].position.y += sin(angle)*2;
                    // if hit other spider than stop idle
                    for(int j=0;j<MAX_SPIDERS;j++){
                        if(i==j)continue;
                        if(myspider[j].active==false)continue;
                        if(rectsoverlap(myspider[i].position.x-4,myspider[i].position.y-4,myspider[i].width+8,myspider[i].height+8,myspider[j].position.x,myspider[j].position.y,myspider[j].width,myspider[j].height)==true) {
                            myspider[i].state=IDLE;
                            myspider[i].position = oldposition;
                            
                        }
                    }
                    if(spidertilecollide(i,0,0)==true){
                            myspider[i].state=IDLE;
                            myspider[i].substate=-1;
                            myspider[i].position = oldposition;                    
                    }
                    // if spider position on player..
                    //if(rectsoverlap(myplayer.position.x,myplayer.position.y,myplayer.width,myplayer.height,myspider[i].position.x,myspider[i].position.y,myspider[i].width,myspider[i].height)==true){
                    //    myspider[i].state=IDLE;
                    //    myspider[i].position = oldposition;
                    //}                        
                    // if spider reaches destination
                    if((abs(myspider[i].target.x-myspider[i].position.x) + abs(myspider[i].target.y-myspider[i].position.y))<4)myspider[i].state=IDLE;
                }
            }
            ///
            //
            //
            // spider checks ahead and places new target (roam slow)
            if(myspider[i].state==SCOUTNEWPOSITION){
                myspider[i].time++;
                if(myspider[i].time>2){
                    myspider[i].frame++;
                    myspider[i].time=0;
                }
                if(myspider[i].frame>2)myspider[i].frame=1;
                //
                if(myspider[i].substate==FINDSPOT){
                    bool newspotisgood=true;
                    float x1=myspider[i].position.x;
                    float y1=myspider[i].position.y;
                    int distance=GetRandomValue(10,150);
                    if(GetRandomValue(0,10)<2)distance=GetRandomValue(5,20);
                    for(int j=0;j<distance;j++){
                        x1+=cos(myspider[i].angle);
                        y1+=sin(myspider[i].angle);
                        //int mx=x1/tileWidth;
                        //int my=y1/tileHeight;
                        if(recttilecollide(x1-10,y1-10,myspider[i].width+5,myspider[i].height+5)==true ||
                           rectsoverlap(myplayer.position.x,myplayer.position.y,myplayer.width,myplayer.height,x1-8,y1-8,myspider[i].width+16,myspider[i].height+16)==true) 
                            {
                            newspotisgood=false;
                            myspider[i].state=IDLE;                            
                        }
                        // if hit other spider than stop idle
                        for(int j=0;j<MAX_SPIDERS;j++){
                            if(i==j)continue;
                            if(myspider[i].active==false)continue;
                            if(rectsoverlap(x1-4,y1-4,myspider[i].width+8,myspider[i].height+8,myspider[j].position.x-4,myspider[j].position.y-4,myspider[j].width+8,myspider[j].height)+8==true) {
                                newspotisgood=false;
                                myspider[i].state=IDLE;                                                            
                            }
                        }                        
                    }
                    if(newspotisgood){
                        myspider[i].target = (Vector2){x1,y1};
                        myspider[i].substate = SPIDERTURN;
                    }
                }
                if(myspider[i].substate==SPIDERTURN){
                    float angle = getangle(myspider[i].position.x,myspider[i].position.y,myspider[i].target.x,myspider[i].target.y);
                    // turn towards target                
                    float difference = angledifference((myspider[i].angle-90)/180*PI,angle);
                    debug=difference;
                    if(difference<0)myspider[i].angle-=6;
                    if(difference>0)myspider[i].angle+=6;
                    if(difference>3)myspider[i].substate=FOUNDSPOT;
                    if(difference==0)myspider[i].substate=FOUNDSPOT;
                }
                if(myspider[i].substate==FOUNDSPOT){
                    float angle = getangle(myspider[i].position.x,myspider[i].position.y,myspider[i].target.x,myspider[i].target.y);
                    Vector2 oldposition = myspider[i].position;
                    myspider[i].position.x += cos(angle)*1;
                    myspider[i].position.y += sin(angle)*1;
                    // if hit other spider than stop idle
                    for(int j=0;j<MAX_SPIDERS;j++){
                        if(i==j)continue;
                        if(myspider[j].active==false)continue;
                        if(rectsoverlap(myspider[i].position.x-4,myspider[i].position.y-4,myspider[i].width+8,myspider[i].height+8,myspider[j].position.x-6,myspider[j].position.y-6,myspider[j].width+12,myspider[j].height+12)==true) {
                            myspider[i].state=IDLE;
                            myspider[i].substate=-1;
                            myspider[i].position = oldposition;
                            
                        }
                    }
                    // if spider position on player..
                    if(rectsoverlap(myplayer.position.x,myplayer.position.y,myplayer.width,myplayer.height,myspider[i].position.x-8,myspider[i].position.y-8,myspider[i].width+16,myspider[i].height+16)==true){
                        myspider[i].state=IDLE;
                        myspider[i].position = oldposition;
                    }                        
                    // if spider reaches destination
                    if((abs(myspider[i].target.x-myspider[i].position.x) + abs(myspider[i].target.y-myspider[i].position.y))<2)myspider[i].state=IDLE;
                }
            }
            
            //
            // Spider charges to target
            if(myspider[i].state==QUICKDASH){
                myspider[i].time++;
                if(myspider[i].time>2){
                    myspider[i].frame++;
                    myspider[i].time=0;
                }
                if(myspider[i].frame>2)myspider[i].frame=1;
                float angle = getangle(myspider[i].position.x,myspider[i].position.y,myspider[i].target.x,myspider[i].target.y);
                // turn towards target                
                float difference = angledifference((myspider[i].angle-90)/180*PI,angle);
                debug=difference;
                if(difference<0)myspider[i].angle-=6;
                if(difference>0)myspider[i].angle+=6;

                if(difference>3){
                myspider[i].position.x += cos(angle)*5;
                myspider[i].position.y += sin(angle)*5;

                if((abs(myspider[i].target.x-myspider[i].position.x) + abs(myspider[i].target.y-myspider[i].position.y))<10)myspider[i].state=IDLE;
                }

                //myspider.angle = (PI/180)*angle;
                //myspider.position.x += cos(angle)*3;
                //myspider.position.y += sin(angle)*3;
                //myspider.angle = angle*180/PI-90;
                //if((abs(myspider.target.x-myspider.position.x) + abs(myspider.target.y-myspider.position.y))<10)myspider.state=0;
            }
        }
        if(IsMouseButtonPressed(0)){
            //myspider[0].target = GetMousePosition();
            //myspider[0].state = 99;
        }
        
        myplayer.idle = true;
        if(IsKeyDown(KEY_RIGHT)&& myplayer.position.x+1<(mapWidth-1)*tileWidth){
            if(playertilecollide(1,0)==false){
                myplayer.position.x+=1;
                myplayer.hdirection = 1;
                myplayer.idle = false;
            }
        }
        if(IsKeyDown(KEY_LEFT) && myplayer.position.x-1>-1){
            if(playertilecollide(-1,0)==false){
                myplayer.position.x-=1;
                myplayer.hdirection = -1;
                myplayer.idle = false;
            }
        }
        if(IsKeyDown(KEY_UP)&& myplayer.position.y-1>-1){
            if(playertilecollide(0,-1)==false){
                myplayer.position.y-=1;
                myplayer.idle = false;
            }
        }
        if(IsKeyDown(KEY_DOWN) && myplayer.position.y+1<(mapHeight-1)*tileHeight){
            if(playertilecollide(0,1)==false){
                myplayer.position.y+=1;
                myplayer.idle = false;
            }
        }        

        // player animations
        if(myplayer.idle==false){
            myplayer.framewalkcount++;
            if(myplayer.framewalkcount>myplayer.framewalkdelay){
                myplayer.framewalkcount = 0;
                myplayer.frameposition++;
                if(myplayer.frameposition>myplayer.framewalkend)myplayer.frameposition=myplayer.framewalkstart;
            }            
        }else{
            myplayer.frameposition = 1;
        }

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            //ClearBackground((Color){60,120,60,255});
            ClearBackground(BLACK);
            // Draw our tilemap.
            for(int y=0;y<10;y++){
                for(int x=0;x<20;x++){
                    if(map[y][x]==1){
                        //DrawRectangle(x*tileWidth,y*tileHeight,tileWidth,tileHeight,LIGHTGRAY);
                    }
                    if(tilemap[y][x]>0){
                        DrawTexturePro(arr_tileset[tilemap[y][x]].tile.texture,       (Rectangle){0,0,arr_tileset[tilemap[y][x]].tile.texture.width,arr_tileset[tilemap[y][x]].tile.texture.height},
                                                                    (Rectangle){x*tileWidth,y*tileHeight,
                                                                    tileWidth,tileHeight},
                                                                    (Vector2){0,0},0,WHITE);                  
                        
                    }



                }
            }
            // DRAW EGGSACKS
            for(int i=0;i<MAX_EGGSACKS;i++){
                if(arr_eggsack[i].state==EGGSACKFULL){
                    DrawTexturePro(spriteeggsackfull.texture,       (Rectangle){0,0,spriteeggsackfull.texture.width,spriteeggsackfull.texture.height},
                                                                    (Rectangle){arr_eggsack[i].position.x,arr_eggsack[i].position.y,
                                                                    arr_eggsack[i].width,arr_eggsack[i].height},
                                                                    (Vector2){arr_eggsack[i].width/2,arr_eggsack[i].height/1.5},0,WHITE);                  

                }
                if(arr_eggsack[i].state==EGGSACKEMPTY){
                    DrawTexturePro(spriteeggsackempty.texture,       (Rectangle){0,0,spriteeggsackempty.texture.width,spriteeggsackempty.texture.height},
                                                                    (Rectangle){arr_eggsack[i].position.x,arr_eggsack[i].position.y,
                                                                    arr_eggsack[i].width,arr_eggsack[i].height},
                                                                    (Vector2){arr_eggsack[i].width/2,arr_eggsack[i].height/1.5},0,WHITE);                  

                }
            }
            //             
            // DRAW SPIDERS
            for(int i=0;i<MAX_SPIDERS;i++){
                if(myspider[i].active==false)continue;
                
                if(myspider[i].frame==1){
                DrawTexturePro(spritespider1.texture,    (Rectangle){0,0,spritespider1.texture.width,spritespider1.texture.height},
                                                        (Rectangle){myspider[i].position.x,myspider[i].position.y,
                                                        myspider[i].width,myspider[i].height},
                                                        (Vector2){myspider[i].width/2,myspider[i].height/1.5},myspider[i].angle,WHITE);                  
                }
                if(myspider[i].frame==2){
                DrawTexturePro(spritespider2.texture,    (Rectangle){0,0,spritespider2.texture.width,spritespider2.texture.height},
                                                        (Rectangle){myspider[i].position.x,myspider[i].position.y,
                                                        myspider[i].width,myspider[i].height},
                                                        (Vector2){myspider[i].width/2,myspider[i].height/1.5},myspider[i].angle,WHITE);                  
                }
            }
            
            int zztop = myplayer.frame[myplayer.frameposition].texture.width;
            if(myplayer.hdirection==-1)zztop=-zztop;
            DrawTexturePro(myplayer.frame[myplayer.frameposition].texture,      (Rectangle){0,0,zztop,myplayer.frame[0].texture.height},
                                                                                (Rectangle){myplayer.position.x,myplayer.position.y,
                                                                                myplayer.width,myplayer.height},
                                                                                (Vector2){0,0},0,WHITE);                  

            //if(myspider[0].active){
            //    DrawText(FormatText("hello: %i",myspider[0].state),10,0,20,WHITE);
            //    DrawText(FormatText("hello: %i",myspider[0].substate),10,15,20,WHITE);
            //    DrawText(FormatText("hello: %f",myspider[0].disttarget),10,30,20,WHITE);
            //    DrawText(FormatText("hello: %f",debug),10,45,20,WHITE);
            //   DrawRectangle(myspider[0].target.x,myspider[0].target.y,5,5,RED);
            //}

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadRenderTexture(spritespider1); 
    UnloadRenderTexture(spritespider2); 
    UnloadRenderTexture(spriteplayer); 
    UnloadRenderTexture(spriteeggsackempty);
    UnloadRenderTexture(spriteeggsackfull);
    for(int i=0;i<MAX_TILES;i++){
        UnloadRenderTexture(arr_tileset[i].tile); 
    }
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;


}


void inisprites(){
    //
    // The sprite data goes here.
int sprite_spider1[8][8] = {
{0,21,21,0,21,21,0,21},
{0,21,0,21,21,0,21,21},
{21,0,0,0,0,21,0,21},
{21,0,0,0,8,0,21,0},
{21,0,0,0,8,0,21,0},
{21,0,0,0,0,21,0,21},
{0,21,0,21,21,0,21,21},
{0,21,+21,0,21,21,0,21}};

int sprite_spider2[8][8] = {
{21,0,21,21,0,21,21,21},
{21,21,0,21,0,21,21,21},
{0,0,0,0,0,21,0,0},
{21,0,0,0,8,0,21,21},
{21,0,0,0,8,0,21,21},
{0,0,0,0,0,21,0,0},
{21,21,0,21,0,21,21,21},
{21,0,21,21,0,21,21,21}};


int sprite_player[8][8] = {
{21,21,21,21,6,21,21,21},
{21,21,21,18,21,21,21,21},
{21,21,20,21,21,2,3,0},
{0,6,19,18,3,21,21,21},
{0,6,19,18,3,21,21,21},
{21,21,20,21,21,2,3,0},
{21,21,21,18,21,21,21,21},
{21,21,21,21,6,21,21,21}};

int sprite_playerw1[8][8] = {
{21,21,21,21,6,21,21,21},
{21,21,21,18,21,21,21,0},
{21,21,20,21,21,21,4,21},
{0,2,19,18,3,2,21,21},
{0,6,19,18,3,21,21,21},
{21,21,20,21,21,2,21,21},
{21,21,21,18,21,21,4,21},
{21,21,21,21,6,21,21,0}};

int sprite_playerw2[8][8] = {
{21,21,21,21,21,21,21,21},
{21,21,21,21,18,6,21,21},
{21,21,20,18,21,21,21,21},
{0,2,19,18,3,2,4,0},
{0,6,19,18,3,2,4,0},
{21,21,20,18,18,21,21,21},
{21,21,21,21,6,21,21,21},
{21,21,21,21,21,21,21,21}};


    
    BeginTextureMode(spritespider1);    
    ClearBackground(BLANK); // Make the entire Sprite Transparent.
    EndTextureMode();
    BeginTextureMode(spritespider2);    
    ClearBackground(BLANK); // Make the entire Sprite Transparent.
    EndTextureMode();
    BeginTextureMode(spriteplayer);    
    ClearBackground(BLANK); // Make the entire Sprite Transparent.
    EndTextureMode();

int sprite_1[8][8] = {
{14,14,14,14,14,14,14,1},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,1,14,14,14,14,14,1},
{14,14,14,14,14,14,14,14},
{14,1,14,14,14,14,14,14},
{14,1,14,14,14,14,14,14},
{1,1,14,14,14,14,14,14}};

int sprite_2[8][8] = {
{14,14,14,14,1,14,1,0},
{31,14,13,13,14,13,13,14},
{31,14,13,13,14,13,13,14},
{31,14,13,13,1,13,13,1},
{31,14,1,1,1,13,13,1},
{31,14,13,13,1,13,13,14},
{31,14,13,13,14,13,13,14},
{31,14,13,13,14,13,13,1}};

int sprite_3[8][8] = {
{14,14,14,14,0,14,0,0},
{6,31,31,31,14,31,31,14},
{31,13,13,13,14,13,13,14},
{31,13,13,13,14,13,13,14},
{14,14,14,14,14,14,14,14},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}};

int sprite_4[8][8] = {
{14,14,14,14,14,14,14,14},
{31,31,31,31,14,31,31,14},
{13,13,13,13,14,13,13,14},
{13,13,13,13,14,13,13,14},
{14,14,14,14,14,14,14,14},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}};

int sprite_5[8][8] = {
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{14,14,14,14,14,14,14,14},
{31,13,13,13,13,14,13,13},
{31,13,13,13,13,14,13,13},
{6,31,31,31,31,14,31,31}};

int sprite_6[8][8] = {
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{14,14,14,14,14,14,14,14},
{13,13,13,13,14,13,13,14},
{13,13,13,13,14,13,13,14},
{31,31,31,31,14,31,31,14}};

int sprite_7[8][8] = {
{14,14,14,1,0,0,0,0},
{31,14,13,14,0,0,0,1},
{31,14,13,14,0,0,0,0},
{31,14,13,14,0,0,0,1},
{31,14,14,1,0,1,14,14},
{31,14,13,14,0,0,0,0},
{31,14,13,14,0,0,0,1},
{31,14,13,14,0,0,0,0}};

int sprite_8[8][8] = {
{31,13,13,14,0,0,0,1},
{6,13,13,14,0,0,0,0},
{4,13,13,14,0,0,0,0},
{13,13,14,0,0,0,0,1},
{14,14,0,0,1,0,1,14},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}};

int sprite_9[8][8] = {
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{14,14,0,0,0,1,0,14},
{13,13,14,0,0,0,0,1},
{4,13,13,14,0,0,0,0},
{6,13,13,14,0,0,0,1}};

int sprite_10[8][8] = {
{0,1,1,1,1,1,1,0},
{31,1,31,7,1,31,1,0},
{3,2,3,3,2,3,2,0},
{3,1,3,3,1,3,1,0},
{0,1,1,1,1,1,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0}};

int sprite_11[8][8] = {
{14,14,14,14,1,14,1,0},
{31,14,13,13,14,13,13,14},
{31,14,21,22,14,13,13,14},
{31,21,1,22,22,13,22,1},
{31,22,1,22,23,13,23,1},
{31,14,22,23,1,13,13,14},
{31,14,13,13,14,13,13,14},
{31,14,13,13,14,13,13,1}};

int sprite_12[8][8] = {
{14,14,14,14,1,14,1,0},
{31,20,5,5,27,13,13,14},
{31,23,5,5,27,27,13,14},
{31,25,8,8,5,5,27,1},
{31,25,8,8,5,5,27,1},
{31,23,5,5,27,27,13,14},
{31,22,5,5,27,13,13,14},
{31,14,13,13,14,13,13,1}};

int sprite_20[8][8] = {
{0,1,14,14,14,1,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,1,14,14},
{0,1,14,14,14,1,14,1},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,1,1,1,1,1,1},
{0,0,0,0,0,0,0,0}};

int sprite_21[8][8] = {
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14}};

int sprite_22[8][8] = {
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{1,1,1,1,1,1,1,1},
{0,0,0,0,0,0,0,0}};

int sprite_23[8][8] = {
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,1,14,14,1,0},
{0,1,14,14,14,14,1,0}};

int sprite_24[8][8] = {
{14,14,14,14,14,14,14,14},
{6,30,31,31,14,31,31,14},
{30,13,13,13,14,13,13,14},
{31,13,13,13,14,13,13,14},
{31,13,13,14,14,14,14,14},
{31,13,13,14,0,0,0,0},
{31,13,13,14,0,0,0,0},
{31,13,13,14,0,0,0,0}};

int sprite_26[8][8] = {
{31,13,13,14,0,0,0,0},
{31,13,13,14,0,0,0,0},
{31,13,13,14,0,0,0,0},
{31,13,13,14,0,0,0,0},
{31,13,13,14,14,14,14,14},
{31,13,13,13,14,13,13,14},
{30,13,13,13,14,13,13,14},
{6,30,31,31,14,31,31,14}};

int sprite_27[8][8] = {
{14,14,14,14,1,14,1,0},
{31,14,13,13,1,13,13,14},
{31,14,13,13,14,1,1,1},
{31,14,13,13,1,13,1,1},
{31,14,2,1,1,13,13,1},
{31,14,31,13,1,13,13,14},
{31,14,13,13,14,4,1,1},
{31,14,13,13,14,13,13,1}};

int sprite_28[8][8] = {
{14,14,14,1,0,0,0,0},
{31,14,13,14,0,0,0,1},
{31,14,13,14,0,0,0,0},
{25,14,13,2,0,0,0,1},
{13,1,2,1,0,1,14,14},
{25,14,13,1,0,0,0,0},
{31,1,13,14,0,0,0,1},
{31,14,13,14,0,0,0,0}};

int sprite_30[8][8] = {
{0,0,0,0,0,0,0,0},
{1,1,1,1,1,2,1,1},
{14,14,14,1,3,3,31,1},
{14,14,14,1,1,2,1,1},
{14,14,14,1,1,2,7,1},
{14,14,14,1,3,3,31,1},
{1,1,1,1,1,2,1,1},
{0,0,0,0,3,3,31,0}};

int sprite_31[8][8] = {
{14,14,14,14,14,14,14,14},
{14,14,14,14,1,14,14,14},
{14,14,1,1,4,1,14,14},
{14,1,7,6,1,3,1,14},
{1,22,25,4,3,1,14,14},
{1,7,6,1,3,1,1,14},
{14,1,6,3,1,6,4,1},
{14,14,1,1,1,4,1,14}};

int sprite_40[8][8] = {
{0,0,0,0,0,0,0,0},
{0,1,1,1,1,1,1,1},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,1,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14}};

int sprite_41[8][8] = {
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,1,1,1,1,1,1},
{0,0,0,0,0,0,0,0}};

int sprite_42[8][8] = {
{0,0,0,0,0,0,0,0},
{1,1,1,1,1,1,1,1},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14}};

int sprite_43[8][8] = {
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14}};

int sprite_44[8][8] = {
{0,0,0,0,0,0,0,0},
{0,1,1,1,1,1,1,0},
{0,1,1,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,1,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0}};

int sprite_45[8][8] = {
{0,0,0,0,0,0,0,0},
{1,1,1,1,1,1,1,1},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{14,14,14,14,14,14,14,14},
{1,1,1,1,1,1,1,1},
{0,0,0,0,0,0,0,0}};

int sprite_47[8][8] = {
{14,14,14,14,14,14,14,14},
{31,25,31,31,14,31,31,14},
{13,13,13,13,1,13,13,14},
{13,13,1,1,14,13,13,14},
{14,14,1,14,14,14,14,14},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}};

int sprite_48[8][8] = {
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{14,1,1,1,14,14,14,14},
{13,13,25,1,14,13,13,14},
{13,13,13,13,1,13,13,14},
{31,31,31,31,14,25,31,14}};

int sprite_60[8][8] = {
{0,0,0,0,0,0,0,0},
{1,1,1,1,1,1,1,0},
{14,14,14,14,14,14,1,0},
{14,14,14,14,14,14,1,0},
{14,1,14,14,14,14,1,0},
{14,14,14,14,14,14,1,0},
{14,14,14,14,1,14,1,0},
{14,14,14,14,14,14,1,0}};

int sprite_61[8][8] = {
{14,14,14,14,14,14,1,0},
{14,14,1,14,14,14,1,0},
{14,14,14,14,14,14,1,0},
{14,14,14,14,14,14,1,0},
{14,14,14,14,14,14,1,0},
{14,14,14,1,14,14,1,0},
{1,1,1,1,1,1,1,0},
{0,0,0,0,0,0,0,0}};

int sprite_62[8][8] = {
{0,0,0,0,0,0,0,0},
{0,1,1,1,1,1,1,1},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,14,14,14},
{0,1,14,14,14,1,14,14},
{0,1,14,14,14,14,14,14},
{0,1,1,1,1,1,1,1},
{0,0,0,0,0,0,0,0}};

int sprite_63[8][8] = {
{14,14,14,14,14,14,1,0},
{14,14,14,14,14,14,1,0},
{14,14,14,14,14,14,1,0},
{14,14,14,14,14,14,1,0},
{14,14,14,14,14,14,1,0},
{14,14,14,14,14,14,1,0},
{14,14,14,14,14,14,1,0},
{14,14,14,14,14,14,1,0}};

int sprite_64[8][8] = {
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,14,14,14,14,1,0},
{0,1,1,1,1,1,1,0},
{0,0,0,0,0,0,0,0}};

int sprite_65[8][8] = {
{0,0,0,0,0,0,0,0},
{1,1,1,1,1,1,1,0},
{14,14,14,14,14,14,1,0},
{14,1,14,14,14,14,1,0},
{14,14,14,14,14,14,1,0},
{14,14,14,14,1,14,1,0},
{1,1,1,1,1,1,1,0},
{0,0,0,0,0,0,0,0}};

int sprite_100[8][8] = { //eggsack full
{14,14,14,0,0,3,0,14},
{14,14,0,3,4,4,3,0},
{14,0,4,4,6,6,4,0},
{4,4,3,4,7,6,6,0},
{7,7,4,6,6,4,6,3},
{14,4,7,6,4,6,6,4},
{14,14,0,7,7,7,7,0},
{14,14,14,0,6,6,0,14}};


int sprite_101[8][8] = { //eggsack empty
{14,14,14,14,0,3,0,14},
{14,14,14,14,0,4,3,0},
{14,14,14,14,14,0,4,0},
{14,14,14,14,14,0,6,0},
{14,14,14,14,14,0,6,3},
{14,14,14,14,14,0,6,4},
{14,14,14,14,0,7,7,0},
{14,14,14,14,0,0,0,14}};


    for(int i=0;i<MAX_TILES;i++){
        BeginTextureMode(arr_tileset[i].tile);    
        ClearBackground(BLANK); // Make the entire Sprite Transparent.
        EndTextureMode();
    }
    
    //db32color[0] = (Color){0,0,0,0};
    // Draw something on it.
    for (int y=0;y<8;y++)
    {
        for (int x=0;x<8; x++)
        {            
                BeginTextureMode(spritespider1);    
                if(sprite_spider1[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_spider1[x][7-y]]);
                EndTextureMode(); 
                BeginTextureMode(spritespider2);    
                if(sprite_spider2[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_spider2[x][7-y]]);
                EndTextureMode(); 

                BeginTextureMode(spriteplayer);    
                if(sprite_player[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_player[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(myplayer.frame[0]);    
                if(sprite_player[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_player[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(myplayer.frame[1]);    
                if(sprite_playerw1[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_playerw1[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(myplayer.frame[2]);    
                if(sprite_playerw2[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_playerw2[x][7-y]]);
                EndTextureMode();
                
                

                BeginTextureMode(arr_tileset[1].tile);    
                if(sprite_1[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_1[x][7-y]]);                
                EndTextureMode();                
                BeginTextureMode(arr_tileset[2].tile);    
                if(sprite_2[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_2[x][7-y]]);                
                EndTextureMode();
                BeginTextureMode(arr_tileset[3].tile);    
                if(sprite_3[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_3[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[4].tile);    
                if(sprite_4[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_4[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[5].tile);    
                if(sprite_5[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_5[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[6].tile);    
                if(sprite_6[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_6[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[7].tile);    
                if(sprite_7[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_7[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[8].tile);    
                if(sprite_8[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_8[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[9].tile);    
                if(sprite_9[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_9[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[10].tile);    
                if(sprite_10[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_10[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[11].tile);    
                DrawRectangle2(x*4,y*4,4,4,db32color[sprite_11[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[12].tile);    
                if(sprite_12[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_12[x][7-y]]);                
                EndTextureMode();
                BeginTextureMode(arr_tileset[20].tile);    
                if(sprite_20[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_20[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[21].tile);    
                //if(sprite_21[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_21[x][7-y]]);
                if(sprite_21[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_21[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[22].tile);    
                if(sprite_22[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_22[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[23].tile);    
                if(sprite_23[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_23[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[24].tile);    
                if(sprite_24[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_24[x][7-y]]);
                EndTextureMode();


                BeginTextureMode(arr_tileset[26].tile);    
                if(sprite_26[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_26[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[27].tile);    
                if(sprite_27[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_27[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[28].tile);    
                if(sprite_28[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_28[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[30].tile);    
                if(sprite_30[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_30[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[31].tile);    
                if(sprite_31[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_31[x][7-y]]);
                EndTextureMode();

                BeginTextureMode(arr_tileset[40].tile);    
                if(sprite_40[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_40[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[41].tile);    
                if(sprite_41[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_41[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[42].tile);    
                if(sprite_42[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_42[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[43].tile);    
                if(sprite_43[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_43[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[44].tile);    
                if(sprite_44[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_44[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[45].tile);    
                if(sprite_45[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_45[x][7-y]]);
                EndTextureMode();

                BeginTextureMode(arr_tileset[47].tile);    
                if(sprite_47[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_47[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[48].tile);    
                if(sprite_48[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_48[x][7-y]]);
                EndTextureMode();

                BeginTextureMode(arr_tileset[60].tile);    
                if(sprite_60[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_60[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[61].tile);    
                if(sprite_61[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_61[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[62].tile);    
                if(sprite_62[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_62[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[63].tile);    
                if(sprite_63[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_63[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[64].tile);    
                if(sprite_64[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_64[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(arr_tileset[65].tile);    
                if(sprite_65[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_65[x][7-y]]);
                EndTextureMode();

                BeginTextureMode(spriteeggsackfull);    
                if(sprite_100[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_100[x][7-y]]);
                EndTextureMode();
                BeginTextureMode(spriteeggsackempty);    
                if(sprite_101[x][7-y]!=21)DrawRectangle2(x*4,y*4,4,4,db32color[sprite_101[x][7-y]]);
                EndTextureMode();


        }
    }

}

void inidb32colors(){		
    db32color[0 ] =  (Color){0      ,0      ,0      ,255};
    db32color[1 ] =  (Color){34     ,32     ,52     ,255};
    db32color[2 ] =  (Color){69     ,40     ,60     ,255};
    db32color[3 ] =  (Color){102    ,57     ,49     ,255};
    db32color[4 ] =  (Color){143    ,86     ,59     ,255};
    db32color[5 ] =  (Color){223    ,113    ,38     ,255};
    db32color[6 ] =  (Color){217    ,160    ,102    ,255};
    db32color[7 ] =  (Color){238    ,195    ,154    ,255};
    db32color[8 ] =  (Color){251    ,242    ,54     ,255};
    db32color[9 ] =  (Color){153    ,229    ,80     ,255};
    db32color[10] =  (Color){106    ,190    ,48     ,255};
    db32color[11] =  (Color){55     ,148    ,110    ,255};
    db32color[12] =  (Color){75     ,105    ,47     ,255};
    db32color[13] =  (Color){82     ,75     ,36     ,255};
    db32color[14] =  (Color){50     ,60     ,57     ,255};
    db32color[15] =  (Color){63     ,63     ,116    ,255};
    db32color[16] =  (Color){48     ,96     ,130    ,255};
    db32color[17] =  (Color){91     ,110    ,225    ,255};
    db32color[18] =  (Color){99     ,155    ,225    ,255};
    db32color[19] =  (Color){95     ,205    ,228    ,255};
    db32color[20] =  (Color){203    ,219    ,252    ,255};
    db32color[21] =  (Color){255    ,255    ,255    ,255};
    db32color[22] =  (Color){155    ,173    ,183    ,255};
    db32color[23] =  (Color){132    ,126    ,135    ,255};
    db32color[24] =  (Color){105    ,106    ,106    ,255};
    db32color[25] =  (Color){89     ,86     ,82     ,255};
    db32color[26] =  (Color){118    ,66     ,138    ,255};
    db32color[27] =  (Color){172    ,50     ,50     ,255};
    db32color[28] =  (Color){217    ,87     ,99     ,255};
    db32color[29] =  (Color){215    ,123    ,186    ,255};
    db32color[30] =  (Color){143    ,151    ,74     ,255};
    db32color[31] =  (Color){138    ,111    ,48     ,255};
}

//Unit collide with solid blocks true/false
bool playertilecollide(int offsetx,int offsety){
    int cx = (myplayer.position.x+offsetx)/tileWidth;
    int cy = (myplayer.position.y+offsety)/tileHeight;
    for(int y2=cy-2; y2<cy+3;y2++){//Note that the - and + are to be set differently with differently sized players
    for(int x2=cx-2; x2<cx+3;x2++){
        if(x2>=0 && x2<mapWidth && y2>=0 && y2<mapHeight){
            if(map[y2][x2] == 1){
                int x3 = (x2)*tileWidth;
                int y3 = (y2)*tileHeight;
                if(rectsoverlap(myplayer.position.x+offsetx,myplayer.position.y+offsety,myplayer.width,myplayer.height,x3,y3,tileWidth,tileHeight)){
                    return true;
                }
            }
        }
    }}
    return false;
}

//Unit collide with solid blocks true/false
bool spidertilecollide(int index, int offsetx,int offsety){
    int cx = (myspider[index].position.x+offsetx)/tileWidth;
    int cy = (myspider[index].position.y+offsety)/tileHeight;
    for(int y2=cy-2; y2<cy+3;y2++){//Note that the - and + are to be set differently with differently sized players
    for(int x2=cx-2; x2<cx+3;x2++){
        if(x2>=0 && x2<mapWidth && y2>=0 && y2<mapHeight){
            if(map[y2][x2] == 1){
                int x3 = (x2)*tileWidth;
                int y3 = (y2)*tileHeight;
                if(rectsoverlap(myspider[index].position.x+offsetx,myspider[index].position.y+offsety,myspider[index].width,myspider[index].height,x3,y3,tileWidth,tileHeight)){
                    return true;
                }
            }
        }
    }}
    return false;
}

//Unit collide with solid blocks true/false
bool recttilecollide(int x,int y,int w, int h){
    
    int cx = (x)/tileWidth;
    int cy = (y)/tileHeight;
    for(int y2=cy-2; y2<cy+3;y2++){//Note that the - and + are to be set differently with differently sized players
    for(int x2=cx-2; x2<cx+3;x2++){
        if(x2>=0 && x2<mapWidth && y2>=0 && y2<mapHeight){
            if(map[y2][x2] == 1){
                int x3 = (x2)*tileWidth;
                int y3 = (y2)*tileHeight;
                if(rectsoverlap(x,y,w,h,x3,y3,tileWidth,tileHeight)){
                    return true;
                }
            }
        }
    }}
    return false;
}



// Rectangles overlap
bool rectsoverlap(int x1,int y1,int w1,int h1,int x2,int y2,int w2,int h2){
    if(x1 >= (x2 + w2) || (x1 + w1) <= x2) return false;
    if(y1 >= (y2 + h2) || (y1 + h1) <= y2) return false;
    return true;
}

static void maketilemap(void){
    for(int y=0;y<mapHeight;y++){
    for(int x=0;x<mapWidth;x++){
        if(map[y][x]==0)tilemap[y][x]=43;
        if(map[y][x]==1)tilemap[y][x]=2;
    }}
 
    for(int y=0;y<mapHeight;y++){
    for(int x=0;x<mapWidth;x++){
        if(x<mapWidth-2){
        if(x>0 && map[y][x-1]==1){
        }else{
        if(map[y][x]==1 && map[y][x+1]==0)tilemap[y][x]=6;
        }
        }
    }}

    
    for(int y=0;y<mapHeight;y++){
    for(int x=0;x<mapWidth;x++){

        if(y<mapHeight-1){//shadow below top wall
            if(map[y][x]==1 && map[y+1][x]==0)tilemap[y+1][x]=21;
        }
        if(y>0){//shadow on bottom wall
            if(map[y][x]==1 && map[y-1][x]==0)tilemap[y-1][x]=63;
        }

        if(x<mapWidth-1){//Shadow on the walls left side
            if(map[y][x]==1 && map[y][x+1]==0)tilemap[y][x+1]=42;
        }
        if(x>0){//Shadow on the walls right side
            if(map[y][x]==1 && map[y][x-1]==0)tilemap[y][x-1]=22;
        }
        
 
    }}
    for(int y=0;y<mapHeight;y++){
    for(int x=0;x<mapWidth;x++){
       if(y<mapHeight-1){//shadow top left
            if(map[y][x]==1 && map[y+1][x]==1 && map[y][x+1]==1 && map[y+1][x+1]==0)tilemap[y+1][x+1]=40;
        }
       if(x<mapWidth && y<mapHeight-1){//shadow top right
            if(map[y][x]==1 && map[y][x-1]==1 && map[y+1][x]==1 && map[y+1][x-1]==0)tilemap[y+1][x-1]=41;
        }
        if(x<mapWidth-1 && y>0){//shadow bottom left
            if(map[y][x]==1 && map[y-1][x]==1 && map[y][x+1]==1 && map[y-1][x+1]==0)tilemap[y-1][x+1]=60;
        }
        if(x>0 && y>0){//shadow bottom right
            if(map[y][x]==1 && map[y-1][x]==1 && map[y][x-1]==1 && map[y-1][x-1]==0)tilemap[y-1][x-1]=61;
        }
        if(y>0 && y<mapHeight-1){//above and below shadow
        if(map[y][x]==0 && map[y-1][x]==1 && map[y+1][x]==1)tilemap[y][x]=23;
        }
        if(x>0 && x<mapWidth-1){//left and right shadow
        if(map[y][x]==0 && map[y][x-1]==1 && map[y][x+1]==1)tilemap[y][x]=45;
        }

    }}
    
    for(int y=0;y<mapHeight;y++){
    for(int x=0;x<mapWidth;x++){
        //if bottom of map and wall than bottom wall
        if(y==mapHeight-1){
            if(map[y][x]==1)tilemap[y][x]=7;
        }
        //top left corner
        if(x<mapWidth && y<mapHeight){
        if(map[y][x]==1 && map[y+1][x+1]==9 && map[y+1][x]==1 && map[y][x+1]==1)tilemap[y][x]=24;
        }
        //top right corner
        if(y-1>0 && x-1>0 && x+1<mapWidth && y+1<mapHeight){
        if(map[y][x]==1 && map[y+1][x-1]==9 && map[y+1][x]==1 && map[y][x-1]==1)tilemap[y][x]=26;
        }
        //top right corner top row
        if(y==0){
        if(y>-1 && x>0 && x+1<mapWidth && y+1<mapHeight){
        if(map[y][x]==1 && map[y+1][x-1]==0 && map[y+1][x]==1 && map[y][x-1]==1)tilemap[y][x]=3;
        }}
        //top left corner top row
        if(y==0){
        if(y>-1 && x>0 && x+1<mapWidth && y+1<mapHeight){
        if(map[y][x]==1 && map[y+1][x+1]==0 && map[y+1][x]==1 && map[y][x+1]==1)tilemap[y][x]=5;
        }}
        //top left corner
        if(y>-1 && x>-1 && x+1<mapWidth && y+1<mapHeight){
        if(map[y][x]==1 && map[y+1][x+1]==0 && map[y+1][x]==1 && map[y][x+1]==1)tilemap[y][x]=5;
        }
        //top right corner
        if(y>-1 && x>-1 && x<mapWidth && y+1<mapHeight){
        if(map[y][x]==1 && map[y+1][x-1]==0 && map[y+1][x]==1 && map[y][x-1]==1)tilemap[y][x]=3;
        }

        //bottom with nothing below and wall left and right
        if(x>0 && y>0 && x+1<mapWidth && y+1<mapHeight){
        if(map[y][x]==1 && map[y+1][x]==9 && map[y][x-1]==1 && map[y][x+1]==1)tilemap[y][x]=7;
        }
        //left side with nothing on the right side
        if(x>0 && y>0 && x+1<mapWidth && y+1<mapHeight){
        if(map[y][x]==1 && map[y][x+1]==9 && map[y+1][x]==1 && map[y-1][x]==1)tilemap[y][x]=4;
        }
        //left side at the edge
        if(x==mapWidth-1){
        if(x>0 && y>0  && y+1<mapHeight){
        if(map[y][x]==1  && map[y+1][x]==1 && map[y-1][x]==1)tilemap[y][x]=4;
        }}

        //bottom left corner cut
        if(x>-1 && y>-1 && y<mapHeight && x<mapWidth){
        if(map[y][x]==1 && map[y-1][x]==1 && map[y][x+1]==1 && map[y-1][x+1]==0)tilemap[y][x]=9;
        }
        //bottom right cornet cut
        if(x>-1 && y>-1 && y<mapHeight && x<mapWidth){
        if(map[y][x]==1 && map[y-1][x]==1 && map[y][x-1]==1 && map[y-1][x-1]==0)tilemap[y][x]=8;
        }
    }}
    
    //a handfull of random wall decorations.
    int num=3;
    int cnt=1000;
    while(num>0){
        cnt--;
        if(cnt<0)num=0;
        int x=GetRandomValue(0,mapWidth);
        int y=GetRandomValue(0,mapHeight);
        if(tilemap[y][x]==2){
            tilemap[y][x]=11+GetRandomValue(0,1);
            num--;
        }
    }
    num=3;cnt=1000;
    while(num>0){
        cnt--;
        if(cnt<0)num=0;

        int x=GetRandomValue(0,mapWidth);
        int y=GetRandomValue(0,mapHeight);
        if(tilemap[y][x]==2){
            tilemap[y][x]=27;
            num--;
        }
    }
    num=3;cnt=1000;
    while(num>0){
        cnt--;
        if(cnt<0)num=0;

        int x=GetRandomValue(0,mapWidth);
        int y=GetRandomValue(0,mapHeight);
        if(tilemap[y][x]==7){
            tilemap[y][x]=28;
            num--;
        }
    }

    num=3;cnt=1000;
    while(num>0){
        cnt--;
        if(cnt<0)num=0;

        int x=GetRandomValue(0,mapWidth);
        int y=GetRandomValue(0,mapHeight);
        if(tilemap[y][x]==4){
            tilemap[y][x]=47;
            num--;
        }
    }
    num=3;cnt=1000;
    while(num>0){
        cnt--;
        if(cnt<0)num=0;

        int x=GetRandomValue(0,mapWidth);
        int y=GetRandomValue(0,mapHeight);
        if(tilemap[y][x]==6){
            tilemap[y][x]=48;
            num--;
        }
    }

 
}

// Return the angle from - to in float
float getangle(float x1,float y1,float x2,float y2){
    return (float)atan2(y2-y1, x2-x1);
}

//
// This is the orientation function. It returns -1 if the point is left of the inputted line.
// 0 if on the same and 1 if on the right of the line.
// aa,bb,point
int orientation(int ax,int ay,int bx, int by, int cx, int cy){
	if(((bx-ax)*(cy-ay)-(by-ay)*(cx-ax))<0)return -1;
    if(((bx-ax)*(cy-ay)-(by-ay)*(cx-ax))>0)return 1;
    return 0;
}

// takes radian iput! <0 is left is shorter else right turn is shorter.
// When it outputs >3 you can asume it aligns with the target(2) angle.
float angledifference(float angle1, float angle2){
    float difference = angle1 - angle2;
    while (difference < -PI){
        difference += (PI*2);
    }
    while (difference > PI){ 
        difference -= (PI*2);
    }
    return difference;

}

// Manhattan Distance (less precise)
float getdistance(float x1,float y1,float x2,float y2){
    return (float)abs(x2-x1)+abs(y2-y1);
}

void DrawRectangle2(int x,int y,int w,int h,Color col){
    if(processGfx){
        Color coldark = col;
        
        Color collight = col;
        collight.r*=1.1;
        collight.g*=1.1;
        collight.b*=1.1;
        if(collight.r>255)collight.r=255;
        if(collight.g>255)collight.g=255;
        if(collight.b>255)collight.b=255;
        DrawRectangle(x,y,4,4,col);
        for(int y1=0;y1<h;y1+=2){
        for(int x1=1;x1<w;x1+=3){
            coldark.a = 100+GetRandomValue(0,100);            
            DrawRectangle(x1+x,y1+y,1,1,coldark);
        }}        
        for(int i=0;i<2;i++){
            DrawRectangle(x+GetRandomValue(0,w),y+GetRandomValue(0,h),1,1,coldark);                
        }
        for(int i=0;i<1;i++){
            if(GetRandomValue(0,5)<1)DrawRectangle(x+GetRandomValue(0,w),y+GetRandomValue(0,h),1,1,BLACK);                
        }

        for(int i=0;i<2;i++){
            DrawRectangle(x+GetRandomValue(0,w),y+GetRandomValue(0,h),1,1,collight);
        }
        
    }else{
        DrawRectangle(x,y,4,4,col);
    }
}
