// Career.cpp: implementation of the CCareer class.
//
//////////////////////////////////////////////////////////////////////

#include "common.h"
#include "debuglog.h"
#include "game.h"
#include "array.h"
#include "EndLevelData.h"
#include "FEPgoodies.h"
#include "data\MissionScripts\onsldef.msl"
#include "Career.h"
#include "music.h"
#include "frontend.h"
#if TARGET == XBOX
#include "memorycard.h"
#include "xboxmemorycard.h"
#endif

CCareer CAREER ;

int CAREER_VERSION = 9 ;

static int level_structure[NUM_LEVELS][5] =     

// elements stored are:-
// word number, index of lower child,  index of higher child,  To update base word if primary complete,  to update base world if secondary compelte

{				
	 
{ 100,  1,		-1,		110 , -1  },   //  0 , 
{ 110,  2,		-1,		-1  , -1  },   //  1 
{ 200,	3,		 4,		211 , 212 },   //  2 
{ 211,	5,		 6,		231 , 232 },   //  3 ,  
{ 212,	5,		 6,		231 , 232 },   //  4 . 
{ 221,	7,		 8,		-1  , -1  },   //  5 .  
{ 222,	7,		 8,		-1  , -1  },   //  6 . 
{ 231,	9,		-1,		-1  , -1  },   //  7 .   
{ 232,	9,		-1,		-1  , -1  },   //  8 .   
{ 300,	10,		11,		311 , 312 },   //  9 .  
{ 311,	12,		13,		321 , 322 },   //  10 . 
{ 312,	12,		13,		321 , 322 },   //  11 . 
{ 321,	14,		15,		-1  , -1  },   //  12 . 
{ 322,	14,		15,		-1  , -1  },   //  13 . 
{ 331,	16,		-1,		-1  , -1  },   //  14. 
{ 332,	16,		-1,		-1  , -1  },   //  15 .
{ 400,	17,		18,		411 , 412 },   //  16 .
{ 411,  19,		20,		431 , 432 },   //  17 . 
{ 412,	19,		20,		431 , 432 },   //  18 . 
{ 421,	21,		22,		-1  , -1  },   //  19 . 
{ 422,	21,		22,		-1  , -1  },   //  20 . 
{ 431,	23,		-1,		-1  , -1  },   //  21 .  
{ 432,	23,		-1,		-1  , -1  },   //  22 .  
{ 500,	24,		25,		-1  , -1  },   //  23 .
{ 511,	26,		27,		-1  , -1  },   //  24 .
{ 512,	28,		29,		-1  , -1  },   //  25 . 
{ 521,	30,		-1,		-1  , -1  },   //  26 . 
{ 522,	30,		-1,		-1  , -1  },   //  27 . 
{ 523,	30,		-1,		-1  , -1  },   //  28 . 
{ 524,	30,		-1,		-1  , -1  },   //  29 . 
{ 600,	31,		32,		-1  , -1  },   //  30 . 
{ 611,	33,		34,		621 , 622 },   //  31 .
{ 612,	33,		34,		621 , 622 },   //  32 .
{ 621,	35,		-1,		-1  , -1  },   //  33 . 
{ 622,	35,		-1,		-1  , -1  },   //  34 . 
{ 700,	36,		-1,		-1  , -1  },   //  35 . 
{ 710,	37,		-1,		720 , -1  },   //  36 . 
{ 720,	38,		39,		731 , 732 },   //  37 . 
{ 731,	40,		-1,		-1  , -1  },   //  38 . 
{ 732,	41,		-1,		-1  , -1  },   //  39 . 
{ 741,	-1,		-1,		-1  , -1  },   //  40 . 
{ 742,	42,		-1,		-1  , -1  },   //  41 . 
{ 800,	-1,		-1,		-1  , -1  },   //  42 . 

};

int num_nodes = 43 ;


//JCL - for goodies.  Needs to be static but not in the career structure
SINT	new_goodie_count = 0;
//BOOL	first_goodie = TRUE;
BOOL	first_goodie = FALSE;

//******************************************************************************************
CCareerNode::CCareerNode()
{
	Blank();
}


//******************************************************************************************
void CCareerNode::Blank()
{
	mLowerLink = -1 ; 
	mHigherLink= -1 ;
	mWorldNumber = 0 ;
	mComplete = FALSE ;
	mNumAttempts = 0;
	SINT i = 0 ;


	// default is make every thing exists
	for (i=0; i < BASE_THINGS_EXISTS_MEM_REQ ; i++)
	{
		mBaseThingsExists[i] = (int) 0xffffffff ;
	}


	mRanking = -1.f;
}




//******************************************************************************************
BOOL CCareerNode::DoesBaseThingExist(int offset)
{
	int i = offset >> 5 ;
	int b = offset & 31 ;

	int m = 1 ;
	if  (b > 0)
	{
		m = m << b;
	}

	return ((mBaseThingsExists[i] & m) != 0) ;
}


//******************************************************************************************
void CCareerNode::SetBaseThingExistTo(int offset, BOOL val)
{
	int i = offset >> 5 ;
	int b = offset & 31 ;


	int m = 1 ;
	if  (b > 0)
	{
		m = m << b;
	}

	if (val == TRUE)
	{
		mBaseThingsExists[i] = mBaseThingsExists[i] | m ;
	}
	else
	{
		int temp = 0xffffffff ;
		int temp1 = (~m) & temp ;

		mBaseThingsExists[i] = mBaseThingsExists[i] & temp1 ;
	}
}


//******************************************************************************************
CCareerNode* CCareerNodeLink::GetToNode() 
{ 
	if (mToNode == -1) return NULL; 
	return CAREER.GetNode(mToNode) ; 
}


//******************************************************************************************
CCareer::CCareer()
{
	// Reset CareerInProgress
	mCareerInProgress = FALSE;

	mSoundVolume=0.8f;
	mMusicVolume=0.9f;
	mControllerConfigurationNum.SetAll(1) ;
	mInvertYAxis.SetAll(FALSE);	
	mVibration.SetAll(TRUE);
}

//******************************************************************************************
CLevelStructure *CCareer::GetLevelStructure()
{
	return (CLevelStructure *)(&level_structure);
}


//******************************************************************************************


void CCareer::Blank()
{
    if ( num_nodes > MAX_NODES)
	{
		LOG.AddMessage("FATAL ERROR: Too many nodes to be stored") ;
		return ;
	}
	
	SINT node_count=0;
	SINT link_count=0; 
	for (node_count=0; node_count< num_nodes; node_count++)
	{
		mNode[node_count].Blank();
		mNode[node_count].mWorldNumber = level_structure[node_count][0] ;
//		mNode[node_count].mIsStartOfNewIsland = FALSE ; (not used anymore)
	
		mNodeLink[link_count].mToNode = level_structure[node_count][1];
		mNodeLink[link_count+1].mToNode = level_structure[node_count][2] ;
		mNodeLink[link_count].mLinkType = CN_NOT_COMPLETE ;
		mNodeLink[link_count+1].mLinkType = CN_NOT_COMPLETE ;

		mNode[node_count].mLowerLink = link_count;
		mNode[node_count].mHigherLink = link_count+1 ;

		link_count+=2;
	}

	CGoodie blank_goodie;
	mGoodies.SetAll(blank_goodie);
	mKilledThings.SetAll(0);
	mSlots.SetAll(0);

	new_goodie_count = 0;
//	first_goodie = TRUE;
	first_goodie = FALSE;

	// The career has been wiped, so reset CareerInProgress flag
	mCareerInProgress = FALSE;

	mIsGod.SetAll(FALSE);

	mPendingExtraGoodies = 0;
	UpdateGoodieStates();
}


//******************************************************************************************
CCareerNode* CCareer::GetNodeFromWorldNo(int world_num)
{
	SINT i = 0;
	for (i = 0 ;i < num_nodes ;i ++)
	{
		if (mNode[i].mWorldNumber == world_num)
		{
			return &mNode[i] ;
		}
	}
	LOG.AddMessage("WARNING: Could not find career node from world number %d", world_num) ;
	return NULL ;
}


//******************************************************************************************
SPtrSet<CCareerNode> CCareerNode::GetChildNodes()
{
	SPtrSet<CCareerNode> return_list ;

	CCareerNodeLink* lower_link = CAREER.GetLink(mLowerLink);
	if (lower_link->mToNode !=-1) return_list.Append(CAREER.GetNode(lower_link->mToNode)) ;

	CCareerNodeLink* higher_link = CAREER.GetLink(mHigherLink);
	if (higher_link->mToNode !=-1) return_list.Append(CAREER.GetNode(higher_link->mToNode)) ;

	return return_list ;
}


//******************************************************************************************
SPtrSet<CCareerNodeLink> CCareerNode::GetChildLinks()
{
	SPtrSet<CCareerNodeLink> return_list ;
	return_list.Append(CAREER.GetLink(mLowerLink));
	return_list.Append(CAREER.GetLink(mHigherLink));
	return return_list;
}


//******************************************************************************************
SPtrSet<CCareerNodeLink> CCareerNode::GetParentLinks()
{
	SPtrSet<CCareerNodeLink> return_list ;
	int node_count;

	// loop through all nodes
	for (node_count=0; node_count< num_nodes; node_count++)
	{
		CCareerNode* node = CAREER.GetNode(node_count) ;
		if (node)
		{
			// loop through all child links
			SPtrSet<CCareerNodeLink> child_links = node->GetChildLinks() ;
			CCareerNodeLink* link ;
			FOR_ALL_ITEMS_IN(child_links, link)
			{
				// if link goes to 'this' node then add to list
				if (CAREER.GetNode(link->mToNode) == this)
				{
					return_list.Append(link) ;
				}
			}
		}
	}
	return return_list ;
}


//******************************************************************************************
BOOL CCareer::DoesBaseThingExist(int world_number, int offset)
{
	// things existing on previouse levels is not supported for multiplayer
	if (GAME.IsMultiplayer()) return TRUE;

	CCareerNode* node = GetNodeFromWorldNo(world_number);

	if (node)
	{
		return node->DoesBaseThingExist(offset) ;
	}

	// if world not known then assume it exists
	return TRUE;
}

//******************************************************************************************
BOOL CCareer::IsWorldLater(
	SINT			inCurrentWorld,
	SINT			inDiesOn)
{
	CCareerNode		*currentNode=GetNodeFromWorldNo(inCurrentWorld);
	CCareerNode		*diesOnNode= GetNodeFromWorldNo(inDiesOn);

	if ((currentNode) && (diesOnNode) && (currentNode!=diesOnNode))
	{
		if (Later(diesOnNode,currentNode))
			return TRUE;
	}

	return FALSE ;
}

//******************************************************************************************
BOOL CCareer::Later(
	CCareerNode		*inDiesOnNode,
	CCareerNode		*inCurrentNode)
{
	if ((inCurrentNode) && (inDiesOnNode))
	{
		if (inCurrentNode==inDiesOnNode)
			return true;

		CCareerNodeLink		*link;
		
		link=&mNodeLink[inDiesOnNode->mLowerLink];
		
		if (link->mToNode!=-1)
		{
			CCareerNode		*lowerNode=&mNode[link->mToNode];

			if (Later(lowerNode,inCurrentNode))
				return TRUE;
		}

		link=&mNodeLink[inDiesOnNode->mHigherLink];

		if (link->mToNode!=-1)
		{
			CCareerNode		*lowerNode=&mNode[link->mToNode];

			if (Later(lowerNode,inCurrentNode))
				return TRUE;
		}
	}

	return FALSE ;
}

//******************************************************************************************

// update career from end level data stucture
void CCareer::Update()
{
	// the below should only happen when a level is won
	if (END_LEVEL_DATA.mFinalState != GAME_STATE_LEVEL_WON)
	{
		UpdateGoodieStates();
		return ;
	}

	if (END_LEVEL_DATA.mFinalState != GAME_STATE_LEVEL_WON) return ;

	LOG.AddMessage("Updating career (world %d completed)", END_LEVEL_DATA.mWorldFinished) ;

	mSlots = END_LEVEL_DATA.mSlots ;

	UpdateThingsKilled();

	CCareerNode* node = GetNodeFromWorldNo(END_LEVEL_DATA.mWorldFinished) ;

	if (node == NULL)
	{
		LOG.AddMessage("FATAL ERROR: Can't update career because can't find node for world %d", END_LEVEL_DATA.mWorldFinished) ;
		return ;
	}

	// Update Level Data
	if (END_LEVEL_DATA.mRanking > node->mRanking)
		node->mRanking = END_LEVEL_DATA.mRanking;

	node->mComplete = TRUE ;

	// Set our CareerInProgress flag since we now have completed at least one level. This
	// flag gets saved out when we save the career data so we don't need to set this in
	// CCareer::Load()
	mCareerInProgress = TRUE;

	// re calc the link structure for the career
	ReCalcLinks() ;
	UpdateGoodieStates();
}



//******************************************************************************************
void CCareer::ReCalcLinks()
{
	CCareerNode* finished_node = GetNodeFromWorldNo(END_LEVEL_DATA.mWorldFinished) ;

	// loop through all child links of the winning node and update them
	CCareerNodeLink* link;
	SPtrSet<CCareerNodeLink> next_links = finished_node->GetChildLinks() ;
	FOR_ALL_ITEMS_IN(next_links, link)
	{
		BOOL higher_link = FALSE ;
		if (link == CAREER.GetLink(finished_node->mHigherLink) )
		{
			higher_link = TRUE ;
		}


		// sorry hack so we don't have to change career (Change of requirements)
		// Suddenly finishing a level might not change the next levels base world stuff 
		//instead might skip over one !! great

		// update base world stuff
		int node_count ;
		for (node_count=0; node_count< num_nodes; node_count++)
		{
			if (mNode[node_count].mWorldNumber ==  END_LEVEL_DATA.mWorldFinished)
			{
				// update lower tier
				if (level_structure[node_count][3] != -1)
				{
					UpdateBaseWorldExistsStuffForNode(GetNodeFromWorldNo(level_structure[node_count][3])) ;
				}

				// update heigher tier
				if (END_LEVEL_DATA.IsAllSecondaryObjectivesComplete() && level_structure[node_count][4] != -1)
				{
					UpdateBaseWorldExistsStuffForNode(GetNodeFromWorldNo(level_structure[node_count][4])) ;
				}
			}
		}

		if (link->mLinkType != CN_COMPLETE)
		{
			BOOL	complete=FALSE;

			// special case
			if (END_LEVEL_DATA.mWorldFinished == 500)
			{
				if ((GetSlot(SLOT_500_ROCKET)) &&
					(link == CAREER.GetLink(finished_node->mHigherLink)))
				{
					complete=TRUE;
				}
				
				if ((GetSlot(SLOT_500_SUB)) &&
					(link != CAREER.GetLink(finished_node->mHigherLink)))
				{
					complete=TRUE;
				}
			}
			// ok only update the higher link if we have completed all the secondary objectives
			else if (higher_link)
			{
				if (END_LEVEL_DATA.IsAllSecondaryObjectivesComplete())
					complete=TRUE;
			}
			else
			{
				complete=TRUE;
			}

			if (complete)
			{
				link->mLinkType = CN_COMPLETE ;

				// ok if the node the link points at has other links which point at it
				// and are also 'complete' then make them 'complete broken'
				CCareerNode* to_node = GetNode(link->mToNode) ;
				if (to_node)
				{
					SPtrSet<CCareerNodeLink> previous_links = to_node->GetParentLinks();
					CCareerNodeLink* previous_link ;
					FOR_ALL_ITEMS_IN(previous_links, previous_link)
					{
						if (previous_link != link && previous_link->mLinkType == CN_COMPLETE)
						{
							previous_link->mLinkType = CN_COMPLETE_BROKEN ;
						}
					}
				}
			}
		}
	}
}


//******************************************************************************************
void CCareer::UpdateBaseWorldExistsStuffForNode(CCareerNode* node)
{
	
	LOG.AddMessage("Updating base things list in world %d", node->mWorldNumber) ;
	SINT i = 0 ;
	for (i=0 ; i < BASE_THINGS_EXISTS_SIZE ; i++)
	{
		node->SetBaseThingExistTo(i, END_LEVEL_DATA.mBaseThingsLeft[i] ) ;
	}
}

//******************************************************************************************
int	CCareer::GetNumKilled(EKilledType type)
{
	return mKilledThings[(int)type] ;
}


//******************************************************************************************
void CCareer::UpdateThingsKilled()
{
	// don't score on traning level
	if (END_LEVEL_DATA.mWorldFinished == 100) return ;

	int i=0;
	for (i=0;i<TK_TOTAL;i++)
	{
		mKilledThings[i]+= END_LEVEL_DATA.mThingsKilled[i] ;
	
		// debug output
		char tt[20] ;
		if (i==0) sprintf(tt,"Aircraft");
		if (i==1) sprintf(tt,"Vehicles");
		if (i==2) sprintf(tt,"Emplacements");
		if (i==3) sprintf(tt,"Infantry");
		if (i==4) sprintf(tt,"Mechs");
		LOG.AddMessage("%-15s killed this level  %d,  Total %d", tt, END_LEVEL_DATA.mThingsKilled[i], mKilledThings[i]) ;
	}
}

//******************************************************************************************
//******************************************************************************************
//******************************************************************************************

//******************************************************************************************
#define GOODIE_NOT_DONE(a) (mGoodies[a].mState <= GS_INSTRUCTIONS)
#define COMPLETE_LEVEL(a) (CAREER.GetNodeFromWorldNo(a)->mComplete == TRUE)
#define SET_GOODIE_NEW(a)  {if (GOODIE_NOT_DONE(a) == TRUE) {mGoodies[a].mState = (EGoodieState) GS_NEW;}} 
#define GRADE_S CGrade('S')
#define GRADE_A CGrade('A')
#define GRADE_B CGrade('B')
#define GRADE_C CGrade('C')
#define GOODIE_UNLOCKED(a) (GetGoodieState(a) >= GS_NEW)
#define COMPLETE_LEVEL_OR_EVO(world_num) (COMPLETE_LEVEL(world_num) || COMPLETE_LEVEL(world_num+1) )
	
#define INFANTRY_DEAD(num) ( CAREER.GetNumKilled(TK_INFANTY) >= goodies[num].GetNumber() )
#define INFANTRY_DEAD2(num) ( CAREER.GetNumKilled(TK_INFANTY) >= goodies[num].GetNumber2() )

#define AIRCRAFT_DEAD(num) ( CAREER.GetNumKilled(TK_AIRCRAFT) >= goodies[num].GetNumber() )
#define AIRCRAFT_DEAD2(num) ( CAREER.GetNumKilled(TK_AIRCRAFT) >= goodies[num].GetNumber2() )

#define VEHICLES_DEAD(num) ( CAREER.GetNumKilled(TK_VEHICLES) >= goodies[num].GetNumber() )
#define VEHICLES_DEAD2(num) ( CAREER.GetNumKilled(TK_VEHICLES) >= goodies[num].GetNumber2() )

#define WALKERS_DEAD(num) ( CAREER.GetNumKilled(TK_MECHS) >= goodies[num].GetNumber() )
#define WALKERS_DEAD2(num) ( CAREER.GetNumKilled(TK_MECHS) >= goodies[num].GetNumber2() )

#define EMPLACEMENTS_DEAD(num) ( CAREER.GetNumKilled(TK_EMPLACEMENTS) >= goodies[num].GetNumber() )
#define EMPLACEMENTS_DEAD2(num) ( CAREER.GetNumKilled(TK_EMPLACEMENTS) >= goodies[num].GetNumber2() )

#define SET_GOODIE_INSTRUCTION(num) if (GOODIE_NOT_DONE(num)) mGoodies[num].mState = GS_INSTRUCTIONS


BOOL TOTAL_A_GRADES(int goodie_num)
{
	int node_count ;
	int found = 0 ;
	CGrade a_grade = GRADE_A;
	for (node_count=0; node_count< num_nodes; node_count++)
	{
		if (CGrade(CAREER.GetGradeFromRanking(CAREER.GetNode(node_count)->mRanking)) >= a_grade) 
		{
			found++;
		}
	}
	if (found >= goodies[goodie_num].GetNumber()) return TRUE ;
	return FALSE ;
}

BOOL TOTAL_C_GRADES(int goodie_num)
{
	int node_count ;
	int found = 0 ;
	CGrade c_grade = GRADE_C;
	for (node_count=0; node_count< num_nodes; node_count++)
	{
		if (CGrade(CAREER.GetGradeFromRanking(CAREER.GetNode(node_count)->mRanking)) >= c_grade)
		{
			found++;
		}
	}
	if (found >= goodies[goodie_num].GetNumber()) return TRUE ;
	return FALSE ;
}

BOOL TOTAL_S_GRADES(int goodie_num)
{
	int node_count ;
	int found = 0 ;
	CGrade s_grade = GRADE_S;
	for (node_count=0; node_count< num_nodes; node_count++)
	{
		if (CGrade(CAREER.GetGradeFromRanking(CAREER.GetNode(node_count)->mRanking)) == s_grade)
		{
			found++;
		}
	}
	if (found >= goodies[goodie_num].GetNumber()) return TRUE ;
	return FALSE ;
}

CGrade GRADE(int world_num)
{
	float r = -9999 ;
	if (COMPLETE_LEVEL(world_num)) 
	{
		CCareerNode* cn = CAREER.GetNodeFromWorldNo(world_num) ;
		if (cn)
		{
			r= cn->mRanking ;
		}
		else
		{
			LOG.AddMessage("Error: no career node for world %d", world_num) ;
		}
	}
	return CAREER.GetGradeFromRanking(r) ;
}


CGrade GRADE_OR_EVO(int world_num)
{
	CGrade r1 = GRADE(world_num); 
	CGrade r2 = GRADE(world_num+1) ;

	if (r1>=r2) return r1 ;
	return r2 ;
}


//******************************************************************************************
SINT	CCareer::CountGoodies()
{
	SINT	c0, tot = 0;

	for (c0 = 0; c0 < MAX_NUM_GOODIES; c0 ++)
	{
		if (GetGoodieState(c0) >= GS_NEW)
			tot ++;
	}

	return tot;
}

//******************************************************************************************
void CCareer::UpdateGoodieStates()
{
	SINT oc = CountGoodies();

	BOOL jcl_fuck_me_its_late = GOODIE_NOT_DONE(0);

	if (COMPLETE_LEVEL(100) ) SET_GOODIE_NEW(0);
	if (GRADE(110) >= GRADE_C )  SET_GOODIE_NEW(1) ;
	if (GOODIE_UNLOCKED(1) && GRADE(200) >= GRADE_C )  SET_GOODIE_NEW(2) ;
	if (GOODIE_UNLOCKED(2) && GRADE_OR_EVO(231) >= GRADE_C )  SET_GOODIE_NEW(3) ;
	if (GOODIE_UNLOCKED(3) && GRADE_OR_EVO(321) >= GRADE_C )  SET_GOODIE_NEW(4) ;
	if (GOODIE_UNLOCKED(4) && GRADE_OR_EVO(321) >= GRADE_C )  SET_GOODIE_NEW(5) ;
	if (GOODIE_UNLOCKED(5) && GRADE_OR_EVO(621) >= GRADE_C )  SET_GOODIE_NEW(6) ;
	if (GOODIE_UNLOCKED(6) && GRADE_OR_EVO(741) >= GRADE_C )  SET_GOODIE_NEW(7) ;
	if (COMPLETE_LEVEL(100) ) SET_GOODIE_NEW(8) ;
	if (COMPLETE_LEVEL_OR_EVO(211) ) SET_GOODIE_NEW(9) ;
	if (COMPLETE_LEVEL(400) ) SET_GOODIE_NEW(10) ;
	if (COMPLETE_LEVEL(710) ) SET_GOODIE_NEW(11) ;
	if (COMPLETE_LEVEL(200) ) SET_GOODIE_NEW(12) ;
	if (COMPLETE_LEVEL_OR_EVO(331) ) SET_GOODIE_NEW(13) ;	
	if (COMPLETE_LEVEL(110) ) SET_GOODIE_NEW(14) ;
	if (COMPLETE_LEVEL_OR_EVO(621) ) SET_GOODIE_NEW(15) ;
	if (GRADE(400) >= GRADE_C  )SET_GOODIE_NEW(16) ;
	if (GRADE(300) >= GRADE_C ) SET_GOODIE_NEW(17) ;
	if (COMPLETE_LEVEL(512) ) SET_GOODIE_NEW(18) ;
	if (COMPLETE_LEVEL_OR_EVO(221) ) SET_GOODIE_NEW(19) ;
	if (COMPLETE_LEVEL_OR_EVO(611) ) SET_GOODIE_NEW(20) ;
	if (COMPLETE_LEVEL_OR_EVO(211) ) SET_GOODIE_NEW(21) ;
	if (COMPLETE_LEVEL(300) ) SET_GOODIE_NEW(22) ;
	if (GRADE(200) >= GRADE_C)  SET_GOODIE_NEW(23) ;
	if (COMPLETE_LEVEL_OR_EVO(331) ) SET_GOODIE_NEW(24) ;
	if (COMPLETE_LEVEL(400) ) SET_GOODIE_NEW(25) ;
	if (GRADE_OR_EVO(221) >= GRADE_C) SET_GOODIE_NEW(26) ;
	if (COMPLETE_LEVEL_OR_EVO(221)  )SET_GOODIE_NEW(27) ;
	if (COMPLETE_LEVEL(200) ) SET_GOODIE_NEW(28) ;
	if (COMPLETE_LEVEL(300) ) SET_GOODIE_NEW(29) ;
	if (COMPLETE_LEVEL_OR_EVO(211) ) SET_GOODIE_NEW(30) ;
	if (COMPLETE_LEVEL_OR_EVO(211) ) SET_GOODIE_NEW(31) ;
	if (COMPLETE_LEVEL(200) ) SET_GOODIE_NEW(32) ;
	if (INFANTRY_DEAD(33))  SET_GOODIE_NEW(33) ;
	if (INFANTRY_DEAD(34))  SET_GOODIE_NEW(34) ;
	if (INFANTRY_DEAD(35))  SET_GOODIE_NEW(35) ;
	if (AIRCRAFT_DEAD(36))  SET_GOODIE_NEW(36) ;
	if (AIRCRAFT_DEAD(37)) SET_GOODIE_NEW(37) ;
	if (AIRCRAFT_DEAD(38))  SET_GOODIE_NEW(38) ;
	if (AIRCRAFT_DEAD(39))  SET_GOODIE_NEW(39) ;
	if (AIRCRAFT_DEAD(40) && INFANTRY_DEAD2(40) ) SET_GOODIE_NEW(40) ;
	if (AIRCRAFT_DEAD(41) && INFANTRY_DEAD2(41) ) SET_GOODIE_NEW(41) ;
	if (VEHICLES_DEAD(42))  SET_GOODIE_NEW(42) ;
	if (VEHICLES_DEAD(43))  SET_GOODIE_NEW(43) ;
	if (VEHICLES_DEAD(44))  SET_GOODIE_NEW(44) ;
	if (VEHICLES_DEAD(45))  SET_GOODIE_NEW(45) ;
	if (COMPLETE_LEVEL(500))  SET_GOODIE_NEW(46) ;
	if (WALKERS_DEAD(47))  SET_GOODIE_NEW(47) ;
	if (WALKERS_DEAD(48))  SET_GOODIE_NEW(48) ;
	if (WALKERS_DEAD(49))  SET_GOODIE_NEW(49) ;
	// goodie 50 is scripted 
	if (WALKERS_DEAD(51))  SET_GOODIE_NEW(51) ;	
	// goodie 52 is scripted
	if (EMPLACEMENTS_DEAD(53) && AIRCRAFT_DEAD2(53))  SET_GOODIE_NEW(53) ;
	if (EMPLACEMENTS_DEAD(54) )  SET_GOODIE_NEW(54) ;
	if (EMPLACEMENTS_DEAD(55) )  SET_GOODIE_NEW(55) ;
	if (EMPLACEMENTS_DEAD(56) && VEHICLES_DEAD2(56))  SET_GOODIE_NEW(56) ;
	if (EMPLACEMENTS_DEAD(57) && AIRCRAFT_DEAD2(57))  SET_GOODIE_NEW(57) ;
	if (GRADE_OR_EVO(331) >= GRADE_A)  SET_GOODIE_NEW(58) ;
	if (GRADE_OR_EVO(431) >= GRADE_A)  SET_GOODIE_NEW(59) ;
	if (COMPLETE_LEVEL_OR_EVO(523) ) SET_GOODIE_NEW(60) ;
	if (GRADE_OR_EVO(521) >= GRADE_A)  SET_GOODIE_NEW(61) ;
	if (GRADE_OR_EVO(523) >= GRADE_A)  SET_GOODIE_NEW(62) ;
	if (AIRCRAFT_DEAD(63) && GRADE_OR_EVO(621) >= GRADE_C)  SET_GOODIE_NEW(63) ;
	if (GRADE_OR_EVO(731) >= GRADE_A)  SET_GOODIE_NEW(64) ;
	if (GRADE(800) >= GRADE_A)  SET_GOODIE_NEW(65) ;
	if (TOTAL_C_GRADES(66))  SET_GOODIE_NEW(66) ;
//	SET_GOODIE_NEW(66) ;
	// goodies  67 to 70   race ones done in script
	if (COMPLETE_LEVEL_OR_EVO(741))  SET_GOODIE_NEW(71) ;
	if (COMPLETE_LEVEL_OR_EVO(741))  SET_GOODIE_NEW(72) ;
	if (COMPLETE_LEVEL_OR_EVO(741)) SET_GOODIE_NEW(73) ;
	if (TOTAL_S_GRADES(74) ) SET_GOODIE_NEW(74) ;
	if (TOTAL_S_GRADES(75) ) SET_GOODIE_NEW(75) ;
	if (TOTAL_S_GRADES(76) ) SET_GOODIE_NEW(76) ;
	if (TOTAL_S_GRADES(77) ) SET_GOODIE_NEW(77) ;

	// Concept art

	if (GRADE(100) >= GRADE_C) SET_GOODIE_NEW(78);
	if (GRADE(110) >= GRADE_C) SET_GOODIE_NEW(79);
	if (GRADE(200) >= GRADE_C) SET_GOODIE_NEW(80);
	if (GRADE(211) >= GRADE_C) SET_GOODIE_NEW(81);
	if (GRADE(212) >= GRADE_C) SET_GOODIE_NEW(82);
	if (GRADE(221) >= GRADE_C) SET_GOODIE_NEW(83);
	if (GRADE(222) >= GRADE_C) SET_GOODIE_NEW(84);
	if (GRADE(231) >= GRADE_C) SET_GOODIE_NEW(85);
	if (GRADE(232) >= GRADE_C) SET_GOODIE_NEW(86);
	if (GRADE(300) >= GRADE_C) SET_GOODIE_NEW(87);
	if (GRADE(311) >= GRADE_C) SET_GOODIE_NEW(88);
	if (GRADE(312) >= GRADE_C) SET_GOODIE_NEW(89);
	if (GRADE(321) >= GRADE_C) SET_GOODIE_NEW(90);
	if (GRADE(322) >= GRADE_C) SET_GOODIE_NEW(91);
	if (GRADE(331) >= GRADE_C) SET_GOODIE_NEW(92);
	if (GRADE(332) >= GRADE_C) SET_GOODIE_NEW(93);	
	if (GRADE(400) >= GRADE_C) SET_GOODIE_NEW(94);
	if (GRADE(411) >= GRADE_C) SET_GOODIE_NEW(95);
	if (GRADE(412) >= GRADE_C) SET_GOODIE_NEW(96);
	if (GRADE(421) >= GRADE_C) SET_GOODIE_NEW(97);
	if (GRADE(422) >= GRADE_C) SET_GOODIE_NEW(98);
	if (GRADE(431) >= GRADE_C) SET_GOODIE_NEW(99);
	if (GRADE(432) >= GRADE_C) SET_GOODIE_NEW(100);
	if (GRADE(500) >= GRADE_C) SET_GOODIE_NEW(101);
	if (GRADE(511) >= GRADE_C) SET_GOODIE_NEW(102);
	if (GRADE(512) >= GRADE_C) SET_GOODIE_NEW(103);
	if (GRADE(521) >= GRADE_C) SET_GOODIE_NEW(104);
	if (GRADE(522) >= GRADE_C) SET_GOODIE_NEW(105);
	if (GRADE(523) >= GRADE_C) SET_GOODIE_NEW(106);
	if (GRADE(524) >= GRADE_C) SET_GOODIE_NEW(107);
	if (GRADE(600) >= GRADE_C) SET_GOODIE_NEW(108);
	if (GRADE(611) >= GRADE_C) SET_GOODIE_NEW(109);
	if (GRADE(612) >= GRADE_C) SET_GOODIE_NEW(110);
	if (GRADE(621) >= GRADE_C) SET_GOODIE_NEW(111);
	if (GRADE(622) >= GRADE_C) SET_GOODIE_NEW(112);
	if (GRADE(700) >= GRADE_C) SET_GOODIE_NEW(113);
	if (GRADE(710) >= GRADE_C) SET_GOODIE_NEW(114);
	if (GRADE(720) >= GRADE_C) SET_GOODIE_NEW(115);
	if (GRADE(731) >= GRADE_C) SET_GOODIE_NEW(116);
	if (GRADE(732) >= GRADE_C) SET_GOODIE_NEW(117);
	if (GRADE(741) >= GRADE_C) SET_GOODIE_NEW(118);
	if (GRADE(742) >= GRADE_C) SET_GOODIE_NEW(119);
	if (GRADE(800) >= GRADE_C) SET_GOODIE_NEW(120);
	
	if (GRADE(100) >= GRADE_B) SET_GOODIE_NEW(121);
	if (GRADE(110) >= GRADE_B) SET_GOODIE_NEW(122);
	if (GRADE(200) >= GRADE_B) SET_GOODIE_NEW(123);
	if (GRADE(211) >= GRADE_B) SET_GOODIE_NEW(124);
	if (GRADE(212) >= GRADE_B) SET_GOODIE_NEW(125);
	if (GRADE(221) >= GRADE_B) SET_GOODIE_NEW(126);
	if (GRADE(222) >= GRADE_B) SET_GOODIE_NEW(127);
	if (GRADE(231) >= GRADE_B) SET_GOODIE_NEW(128);
	if (GRADE(232) >= GRADE_B) SET_GOODIE_NEW(129);
	if (GRADE(300) >= GRADE_B) SET_GOODIE_NEW(130);
	if (GRADE(311) >= GRADE_B) SET_GOODIE_NEW(131);
	if (GRADE(312) >= GRADE_B) SET_GOODIE_NEW(132);
	if (GRADE(321) >= GRADE_B) SET_GOODIE_NEW(133);
	if (GRADE(322) >= GRADE_B) SET_GOODIE_NEW(134);
	if (GRADE(331) >= GRADE_B) SET_GOODIE_NEW(135);
	if (GRADE(332) >= GRADE_B) SET_GOODIE_NEW(136);	
	if (GRADE(400) >= GRADE_B) SET_GOODIE_NEW(137);
	if (GRADE(411) >= GRADE_B) SET_GOODIE_NEW(138);
	if (GRADE(412) >= GRADE_B) SET_GOODIE_NEW(139);
	if (GRADE(421) >= GRADE_B) SET_GOODIE_NEW(140);
	if (GRADE(422) >= GRADE_B) SET_GOODIE_NEW(141);
	if (GRADE(431) >= GRADE_B) SET_GOODIE_NEW(142);
	if (GRADE(432) >= GRADE_B) SET_GOODIE_NEW(143);
	if (GRADE(500) >= GRADE_B) SET_GOODIE_NEW(144);
	if (GRADE(511) >= GRADE_B) SET_GOODIE_NEW(145);
	if (GRADE(512) >= GRADE_B) SET_GOODIE_NEW(146);
	if (GRADE(521) >= GRADE_B) SET_GOODIE_NEW(147);
	if (GRADE(522) >= GRADE_B) SET_GOODIE_NEW(148);
	if (GRADE(523) >= GRADE_B) SET_GOODIE_NEW(149);
	if (GRADE(524) >= GRADE_B) SET_GOODIE_NEW(150);
	if (GRADE(600) >= GRADE_B) SET_GOODIE_NEW(151);
	if (GRADE(611) >= GRADE_B) SET_GOODIE_NEW(152);
	if (GRADE(612) >= GRADE_B) SET_GOODIE_NEW(153);
	if (GRADE(621) >= GRADE_B) SET_GOODIE_NEW(154);
	if (GRADE(622) >= GRADE_B) SET_GOODIE_NEW(155);
	if (GRADE(700) >= GRADE_B) SET_GOODIE_NEW(156);
	if (GRADE(710) >= GRADE_B) SET_GOODIE_NEW(157);
	if (GRADE(720) >= GRADE_B) SET_GOODIE_NEW(158);
	if (GRADE(731) >= GRADE_B) SET_GOODIE_NEW(159);
	if (GRADE(732) >= GRADE_B) SET_GOODIE_NEW(160);
	if (GRADE(741) >= GRADE_B) SET_GOODIE_NEW(161);
	if (GRADE(742) >= GRADE_B) SET_GOODIE_NEW(162);
	if (GRADE(800) >= GRADE_B) SET_GOODIE_NEW(163);

	if (GRADE(100) >= GRADE_A) SET_GOODIE_NEW(164);
	if (GRADE(110) >= GRADE_A) SET_GOODIE_NEW(165);
	if (GRADE(200) >= GRADE_A) SET_GOODIE_NEW(166);
	if (GRADE(211) >= GRADE_A) SET_GOODIE_NEW(167);
	if (GRADE(212) >= GRADE_A) SET_GOODIE_NEW(168);
	if (GRADE(221) >= GRADE_A) SET_GOODIE_NEW(169);
	if (GRADE(222) >= GRADE_A) SET_GOODIE_NEW(170);
	if (GRADE(231) >= GRADE_A) SET_GOODIE_NEW(171);
	if (GRADE(232) >= GRADE_A) SET_GOODIE_NEW(172);
	if (GRADE(300) >= GRADE_A) SET_GOODIE_NEW(173);
	if (GRADE(311) >= GRADE_A) SET_GOODIE_NEW(174);
	if (GRADE(312) >= GRADE_A) SET_GOODIE_NEW(175);
	if (GRADE(321) >= GRADE_A) SET_GOODIE_NEW(176);
	if (GRADE(322) >= GRADE_A) SET_GOODIE_NEW(177);
	if (GRADE(331) >= GRADE_A) SET_GOODIE_NEW(178);
	if (GRADE(332) >= GRADE_A) SET_GOODIE_NEW(179);	
	if (GRADE(400) >= GRADE_A) SET_GOODIE_NEW(180);
	if (GRADE(411) >= GRADE_A) SET_GOODIE_NEW(181);
	if (GRADE(412) >= GRADE_A) SET_GOODIE_NEW(182);
	if (GRADE(421) >= GRADE_A) SET_GOODIE_NEW(183);
	if (GRADE(422) >= GRADE_A) SET_GOODIE_NEW(184);
	if (GRADE(431) >= GRADE_A) SET_GOODIE_NEW(185);
	if (GRADE(432) >= GRADE_A) SET_GOODIE_NEW(186);
	if (GRADE(500) >= GRADE_A) SET_GOODIE_NEW(187);
	if (GRADE(511) >= GRADE_A) SET_GOODIE_NEW(188);
	if (GRADE(512) >= GRADE_A) SET_GOODIE_NEW(189);
	if (GRADE(521) >= GRADE_A) SET_GOODIE_NEW(190);
	if (GRADE(522) >= GRADE_A) SET_GOODIE_NEW(191);
	if (GRADE(523) >= GRADE_A) SET_GOODIE_NEW(192);
	if (GRADE(524) >= GRADE_A) SET_GOODIE_NEW(193);
	if (GRADE(600) >= GRADE_A) SET_GOODIE_NEW(194);
	if (GRADE(611) >= GRADE_A) SET_GOODIE_NEW(195);
	if (GRADE(612) >= GRADE_A) SET_GOODIE_NEW(196);
	if (GRADE(621) >= GRADE_A) SET_GOODIE_NEW(197);
	if (GRADE(622) >= GRADE_A) SET_GOODIE_NEW(198);
	if (GRADE(700) >= GRADE_A) SET_GOODIE_NEW(199);
	if (GRADE(710) >= GRADE_A) SET_GOODIE_NEW(200);

	SINT nc = CountGoodies();

	new_goodie_count += (nc - oc) + mPendingExtraGoodies;
	
	if (((!(GOODIE_NOT_DONE(0))) && (jcl_fuck_me_its_late)) || ((mPendingExtraGoodies - oc == 0) && (oc != 0)))
		first_goodie = TRUE;

	mPendingExtraGoodies = 0;

	if (IsEpisodeAvailable(1))
	{
		// 0 1 8 14 33 36 41 42 43 
		SET_GOODIE_INSTRUCTION(0);
		SET_GOODIE_INSTRUCTION(1);
		SET_GOODIE_INSTRUCTION(8);
		SET_GOODIE_INSTRUCTION(14); 
		SET_GOODIE_INSTRUCTION(33);
		SET_GOODIE_INSTRUCTION(36);
		SET_GOODIE_INSTRUCTION(41);
		SET_GOODIE_INSTRUCTION(42);
		SET_GOODIE_INSTRUCTION(43);
	}
	

	if (IsEpisodeAvailable(2))
	{
		//	2 3 9 12 19 21 23 26 27 28 30 31 32 35 38 40 
		SET_GOODIE_INSTRUCTION(2);
		SET_GOODIE_INSTRUCTION(3);
		SET_GOODIE_INSTRUCTION(9);
		SET_GOODIE_INSTRUCTION(12);
		SET_GOODIE_INSTRUCTION(19);
		SET_GOODIE_INSTRUCTION(21);
		SET_GOODIE_INSTRUCTION(23);
		SET_GOODIE_INSTRUCTION(26);
		SET_GOODIE_INSTRUCTION(27);
		SET_GOODIE_INSTRUCTION(28);
		SET_GOODIE_INSTRUCTION(30);
		SET_GOODIE_INSTRUCTION(31);
		SET_GOODIE_INSTRUCTION(32);
		SET_GOODIE_INSTRUCTION(35);
		SET_GOODIE_INSTRUCTION(38);
		SET_GOODIE_INSTRUCTION(40);
	}

	if (IsEpisodeAvailable(3))
	{
		// 4 5 13 17 22 24 29 34 39 43 45 47 53 54 55 56 57 58

		SET_GOODIE_INSTRUCTION(4);
		SET_GOODIE_INSTRUCTION(5);
		SET_GOODIE_INSTRUCTION(13);	
		SET_GOODIE_INSTRUCTION(17);
		SET_GOODIE_INSTRUCTION(22);
		SET_GOODIE_INSTRUCTION(24);
		SET_GOODIE_INSTRUCTION(29);
		SET_GOODIE_INSTRUCTION(34);
		SET_GOODIE_INSTRUCTION(39);
		SET_GOODIE_INSTRUCTION(43);
		SET_GOODIE_INSTRUCTION(45);
		SET_GOODIE_INSTRUCTION(47);	
		SET_GOODIE_INSTRUCTION(53);
		SET_GOODIE_INSTRUCTION(54);
		SET_GOODIE_INSTRUCTION(55);
		SET_GOODIE_INSTRUCTION(56);
		SET_GOODIE_INSTRUCTION(57);
		SET_GOODIE_INSTRUCTION(58);
	}

	if (IsEpisodeAvailable(4))
	{
		//10 16 25 44 48 52 59 
		SET_GOODIE_INSTRUCTION(10);
		SET_GOODIE_INSTRUCTION(16);
		SET_GOODIE_INSTRUCTION(25);
		SET_GOODIE_INSTRUCTION(44);
		SET_GOODIE_INSTRUCTION(48);
		SET_GOODIE_INSTRUCTION(52);
		SET_GOODIE_INSTRUCTION(59);
	}

	if (IsEpisodeAvailable(5))
	{
		//18 46 49 50 51 52 60 61 62  
		SET_GOODIE_INSTRUCTION(18);
		SET_GOODIE_INSTRUCTION(46);
		SET_GOODIE_INSTRUCTION(49);
		SET_GOODIE_INSTRUCTION(50);
		SET_GOODIE_INSTRUCTION(51);
		SET_GOODIE_INSTRUCTION(52);
		SET_GOODIE_INSTRUCTION(60);
		SET_GOODIE_INSTRUCTION(61);
		SET_GOODIE_INSTRUCTION(62);
	}

	if (IsEpisodeAvailable(6))
	{
		//	6 15 20 37 63 64 
		SET_GOODIE_INSTRUCTION(6);
		SET_GOODIE_INSTRUCTION(15);
		SET_GOODIE_INSTRUCTION(20);
		SET_GOODIE_INSTRUCTION(37);
		SET_GOODIE_INSTRUCTION(63);
		SET_GOODIE_INSTRUCTION(64);
	}

	if (IsEpisodeAvailable(7))
	{
		//	7 11 64 66 67 68 69 70 71 74 75 76 77 78 
		SET_GOODIE_INSTRUCTION(7);
		SET_GOODIE_INSTRUCTION(11);
		SET_GOODIE_INSTRUCTION(64);
		SET_GOODIE_INSTRUCTION(66);
		SET_GOODIE_INSTRUCTION(67);
		SET_GOODIE_INSTRUCTION(68);
		SET_GOODIE_INSTRUCTION(69);
		SET_GOODIE_INSTRUCTION(70);
		SET_GOODIE_INSTRUCTION(71);
		SET_GOODIE_INSTRUCTION(74);
		SET_GOODIE_INSTRUCTION(75);
		SET_GOODIE_INSTRUCTION(76);
		SET_GOODIE_INSTRUCTION(77);
		SET_GOODIE_INSTRUCTION(78);
	}

	if (IsEpisodeAvailable(8))
	{
		//65 72 73 
		SET_GOODIE_INSTRUCTION(65);
		SET_GOODIE_INSTRUCTION(72);
		SET_GOODIE_INSTRUCTION(73);
	}
}



//******************************************************************************************
//******************************************************************************************
//
// loading and saving, differs on XBOX
//
//******************************************************************************************
//******************************************************************************************

#if TARGET == PC
void CCareer::Load()
{

	CMEMBUFFER buffer ;
	BOOL loaded = buffer.InitFromFile("c:\\dev\\onslaught2\\career.dat") ;
	if (loaded)
	{
		int version = 0;
		buffer.Read(&version,4) ;
		if (version != CAREER_VERSION)
		{
			LOG.AddMessage("FATAL ERROR: Can't load career (wrong version)") ;
			buffer.Close() ;
			return ;
		}

		buffer.Read(this, sizeof(CCareer) ) ;
		buffer.Close() ;
		LOG.AddMessage("Loaded: career.dat");
	}
	mCareerInProgress = TRUE;
	MUSIC.SetVolume(CAREER.GetMusicVolume()) ;
	SOUND.SetMasterVolume(CAREER.GetSoundVolume()) ;

	// point the player at the highest level available
	FRONTEND.mFEPLevelSelect.SetCurrentLevelToHighestAvailable();
}

void CCareer::Save()
{
	// if you're saving it, it's probably worth deeming as in progress
	mCareerInProgress = TRUE;

	CMEMBUFFER buffer ;
	buffer.InitFromMem("c:\\dev\\onslaught2\\career.dat") ;
	buffer.Write(&CAREER_VERSION, 4) ;
	buffer.Write(this, sizeof(CCareer));
	buffer.Close() ;
	LOG.AddMessage("Written: career.dat");

}

#else

// we'll create a version stamp for the savegame which is a function of the hand-crafted
// CAREER_VERSION and the sizeof the structure
static SWORD current_version_stamp()
{
	return SWORD(CAREER_VERSION + (sizeof(CCareer) << 4));
}

//******************************************************************************************

// On XBOX we use this function to copy data from our load buffer (in XBOXFEPLoadGame) into
// our career class. LoadGame handles the CRC-check of the data.
BOOL CCareer::Load(char *source, bool suggest_next_level)
{
	ASSERT(source);

	// the savegame is version checked.
	SWORD saved_version = *(SWORD *)source;

	if (saved_version != current_version_stamp())
	{
		// We've got trouble, the version number is out of date.
		return FALSE;
	}

	// Otherwise, just copy the data straight in, from just after the version number.
	source += sizeof(saved_version);

	memcpy(this, source, sizeof(CCareer));
	MUSIC.SetVolume(CAREER.GetMusicVolume()) ;
	SOUND.SetMasterVolume(CAREER.GetSoundVolume()) ;

	if (suggest_next_level)
	{
		// point the player at the highest level available
		FRONTEND.mFEPLevelSelect.SetCurrentLevelToHighestAvailable();
	}

	return TRUE;
}

//******************************************************************************************

// On XBOX we use this function to copy data from the career class into our save buffer
// (in XBOXFEPSaveGame)
void CCareer::Save(char *dest)
{
	// if you're saving it, it's probably worth deeming as in progress
	mCareerInProgress = TRUE;

	ASSERT(dest);

	// let's put a version number in first.
	SWORD *version_number = (SWORD *)dest;

	*version_number = current_version_stamp();

	// So we'd better advance "dest" beyond the version number.
	dest += sizeof(SWORD);

	// and then just dump the career structure in.
	memcpy(dest, this, sizeof(CCareer));
}

//******************************************************************************************

// And since we're using this buffer to load and save games, we'd better know what size
// it is.
int CCareer::SizeOfSaveGame()
{
	// we're saving an extra int for version info.
	int retval = sizeof(CCareer) + sizeof(SWORD);

#if TARGET == XBOX
	// that's what you think. We're going to make it as many blocks as we're claiming
	// in the translated text so as to not confused people.
	retval += XBOX_BLOCK_SIZE;
#endif

	return retval;
}

#endif

//******************************************************************************************
//******************************************************************************************
//
// end of loading and saving section
//
//******************************************************************************************
//******************************************************************************************



//******************************************************************************************
WCHAR	CCareer::GetGradeFromRanking(float f)
{
	// grade from A - F, say
	char	c;

	if (f == 1.f)
		c = 'S';
	else
	{
		if (f <= 0.f)
			c = 'E';
		else
		{
			SINT i = SINT(floorf(f * 4.f));

			c = 'D' - i;
		}
	}

	char str[2];
	str[0] = c;
	str[1] = 0;

	return ToWCHAR(str)[0];
}




//******************************************************************************************
//******************************************************************************************
//******************************************************************************************
// debug stuff


void CCareer::Log()
{
	LOG.AddMessage("---------------level status------------------");
	PrintNodesStatus() ;
	LOG.AddMessage("---------------goodie values------------------");
	SINT i=0 ;
	for (i=0;i<MAX_NUM_GOODIES;i++)
	{
		char temp[50] = "\0" ;
		if (mGoodies[i].mState == GS_UNKNOWN) sprintf(temp,"UNKNOWN") ;
		if (mGoodies[i].mState == GS_INSTRUCTIONS) sprintf(temp,"INSTRUCTIONS") ;
		if (mGoodies[i].mState == GS_NEW) sprintf(temp,"NEW") ;
		if (mGoodies[i].mState == GS_OLD) sprintf(temp,"OLD") ;

		LOG.AddMessage("Goodie %d value = %s", i, temp) ;
	}

	LOG.AddMessage("---------------killed things values------------------");
	for (i=0;i<TK_TOTAL;i++)
	{
		// debug output
		char tt[20] ;
		if (i==0) sprintf(tt,"Aircraft");
		if (i==1) sprintf(tt,"Vehicles");
		if (i==2) sprintf(tt,"Emplacements");
		if (i==3) sprintf(tt,"Infantry");
		if (i==4) sprintf(tt,"Mechs");
		LOG.AddMessage("%-15s killed %d", tt, mKilledThings[i]) ;
	}

		LOG.AddMessage("---------------slots------------------");
	for (i=0;i<MAX_CAREER_SLOTS*8;i++)
	{
		int ii = i >> 5 ;
		int b = i & 31 ;

		int m = 1 ;
		if  (b > 0)
		{
			m = m << b;
		}
		if ((mSlots[ii] & m) != 0)
		{
			LOG.AddMessage("slot %d is TRUE",i) ;
		}
		else
		{
			LOG.AddMessage("slot %d is FALSE",i) ;
		}
	}


	LOG.AddMessage("------------ configuration --------------") ;

	LOG.AddMessage("Sound Volume = %2.4f", mSoundVolume) ;
	LOG.AddMessage("Music Volume = %2.4f", mMusicVolume) ;

	for (i=0;i<mIsGod.Size();i++)
	{
		if (mIsGod[i] == TRUE)
		{
			LOG.AddMessage("Is god for player '%d' = TRUE", i) ;
		}
		else
		{
			LOG.AddMessage("Is god for player '%d' = FALSE", i) ;
		}
	}

	for (i=0;i<mInvertYAxis.Size();i++)
	{
		if (mInvertYAxis[i] == TRUE)
		{
			LOG.AddMessage("Invert Y axis for player '%d' = TRUE", i) ;
		}
		else
		{
			LOG.AddMessage("Invert Y Axis for player '%d' = FALSE", i) ;
		}
	}

	for (i=0;i<mVibration.Size();i++)
	{
		if (mVibration[i] == TRUE)
		{
			LOG.AddMessage("Vibration for player '%d' = TRUE", i) ;
		}
		else
		{
			LOG.AddMessage("Vibration for player '%d' = FALSE", i) ;
		}
	}

	for(i=0;i<mControllerConfigurationNum.Size();i++)
	{
		LOG.AddMessage("Controller configuration for player '%d' = %d", i, mControllerConfigurationNum[i]) ;
	}
}





//******************************************************************************************
void CCareer::PrintNodesStatus()
{
	int i = 0 ;
	for (i=0 ;i <NUM_LEVELS;i++)
	{
		CCareerNode* node = GetNodeFromWorldNo(level_structure[i][0]) ;
		if (node)
		{
			LOG.AddString("W %3d", node->mWorldNumber) ;
			if (node->mComplete)
			{
				LOG.AddString("(Complete)");
			}
			else
			{
				LOG.AddString("(Blank)   ");
			}

			SPtrSet<CCareerNodeLink> child_links = node->GetChildLinks() ;
			CCareerNodeLink* link;
			FOR_ALL_ITEMS_IN(child_links, link)
			{
				CCareerNode* child_node = link->GetToNode() ;
				if (child_node) 
				{
					LOG.AddString("  Link to %3d = ", child_node->mWorldNumber) ;
					switch (link->mLinkType)
					{
						case CN_COMPLETE : 				LOG.AddString("COMPLETE        ") ; break ;
						case CN_NOT_COMPLETE:			LOG.AddString("NOT COMPLETE    ") ; break ;
						case CN_COMPLETE_BROKEN:		LOG.AddString("COMPLETE BROKEN "); break ;
					}
				}
			}
			LOG.AddMessage("");
		}
	}
}

//******************************************************************************************
BOOL	CCareer::GetSlot(int num)
{
	if (num <0 || num >= MAX_CAREER_SLOTS*8)
	{
		LOG.AddMessage("Error: Outside slot range (%d) in call to GetSlot", num) ;
		return FALSE ;
	}
	int i = num >> 5 ;
	int b = num & 31 ;

	int m = 1 ;
	if  (b > 0)
	{
		m = m << b;
	}

	return ((mSlots[i] & m) != 0) ;

}


//******************************************************************************************	v
void	CCareer::SetSlot(int num, BOOL val)
{
	if (num <0 || num >= MAX_CAREER_SLOTS*8)
	{
		LOG.AddMessage("Error: Outside slot range (%d) in call to SetSlot", num) ;
		return ;
	}

	int i = num >> 5 ;
	int b = num & 31 ;

	int m = 1 ;
	if  (b > 0)
	{
		m = m << b;
	}

	if (val == TRUE)
	{
		mSlots[i] = mSlots[i] | m ;
	}
	else
	{
		int temp = 0xffffffff ;
		int temp1 = (~m) & temp ;

		mSlots[i] = mSlots[i] & temp1 ;
	}

}

//******************************************************************************************
SINT	CCareer::GetAndResetGoodieNewCount()
{
	SINT v = new_goodie_count;
	new_goodie_count = 0;
	return v;
}

//******************************************************************************************
BOOL	CCareer::GetAndResetFirstGoodie()
{
	BOOL	b = first_goodie;
	first_goodie = FALSE;
	return b;
}


//******************************************************************************************
BOOL	CCareer::IsEpisodeAvailable(SINT ep)
{
	switch (ep)
	{
		case 0: case 1: return TRUE ;
		case 2: return COMPLETE_LEVEL(110) ;
		case 3: return COMPLETE_LEVEL(231) || COMPLETE_LEVEL(232) ;
		case 4: return COMPLETE_LEVEL(331) || COMPLETE_LEVEL(332) ;
		case 5: return COMPLETE_LEVEL(431) || COMPLETE_LEVEL(432) ;
		case 6: return COMPLETE_LEVEL(521) || COMPLETE_LEVEL(522) || COMPLETE_LEVEL(523) || COMPLETE_LEVEL(524)  ;
		case 7: return COMPLETE_LEVEL(621) || COMPLETE_LEVEL(622) ;
		case 8: return COMPLETE_LEVEL(741) || COMPLETE_LEVEL(742) ;
		default: return FALSE ;
	}
	return FALSE ;
}



/*
//******************************************************************************************
void   CCareer::Test()
{
	int test_num=0;
	LOG.AddMessage("Career test....");
	LOG.AddMessage("blanking career");
	CAREER.Blank() ;

	if (++test_num && GetGoodieState(1)== GS_UNKNOWN) LOG.AddMessage("Test %d passed",test_num) ; else LOG.AddMessage("Test %d failed", test_num);
	LOG.AddMessage("winning level 100");
	END_LEVEL_DATA.mRanking = 1.0f ;
	END_LEVEL_DATA.mFinalState = GAME_STATE_LEVEL_WON ;
	END_LEVEL_DATA.mWorldFinished = 100 ;
	END_LEVEL_DATA.mThingsKilled.SetAll(0);

	Update() ;

	if (++test_num && GetGoodieState(1)== GS_NEW) LOG.AddMessage("Test %d passed",test_num) ; else LOG.AddMessage("Test %d failed", test_num);
	PrintNodesStatus();

	LOG.AddMessage("winning level 110");
	END_LEVEL_DATA.mWorldFinished = 110 ;
	Update() ;
	PrintNodesStatus() ;

	LOG.AddMessage("winning level 200");
	END_LEVEL_DATA.mWorldFinished = 200 ;
	Update() ;
	PrintNodesStatus() ;

	LOG.AddMessage("winning level 211");
	END_LEVEL_DATA.mWorldFinished = 211 ;
	Update() ;
	PrintNodesStatus() ;

	LOG.AddMessage("winning level 221");
	END_LEVEL_DATA.mWorldFinished = 221 ;
	Update() ;
	PrintNodesStatus() ;

	LOG.AddMessage("winning level 231");
	END_LEVEL_DATA.mWorldFinished = 231 ;
	Update() ;
	PrintNodesStatus() ;

	LOG.AddMessage("winning level 212");
	END_LEVEL_DATA.mWorldFinished = 212 ;
	Update() ;
	PrintNodesStatus() ;

	END_LEVEL_DATA.mSecondaryObjectives[0].Set(MOS_FAILED,1);
	LOG.AddMessage("winning level 222 only lower");
	END_LEVEL_DATA.mWorldFinished = 222 ;
	Update() ;
	PrintNodesStatus() ;

	LOG.AddMessage("winning level 232");
	END_LEVEL_DATA.mWorldFinished = 232 ;
	Update() ;
	PrintNodesStatus() ;

	END_LEVEL_DATA.mSecondaryObjectives[0].Set(MOS_COMPLETE,1);
	LOG.AddMessage("winning level 222 now higher aswell");
	END_LEVEL_DATA.mWorldFinished = 222 ;
	Update() ;
	PrintNodesStatus() ;

	LOG.AddMessage("winning level 211 again to remove broken links");
	END_LEVEL_DATA.mWorldFinished = 211 ;
	Update() ;
	PrintNodesStatus() ;
}
*/


