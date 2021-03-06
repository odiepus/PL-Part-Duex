#include "cs3723p1.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/****************************************************
 cs3723p1.c by Hector Herrera
Purpose:
  The code in this file are the student defined functions required
  to run the program in the Storage Management program in 
  cs3723p1Driver.c provided by Professor Larry Clark.

  *************************************************/
/***************FreeNode***********************
FreeNode *searchFreeListForNodes(StorageManager *pMgr, short shTotalNodeSizeNeeded, short shNodeType, SMResult *psmResult)

Purpose:
  This function looks inside the free node list in pMgr and searches for a free node
  based on node type and size. It will only choose a node that is the same size or 
  greater. If node is found then its address is returned and the free node list is 
  updated.
Parameters:
  I   StorageManager *pMgr Contains node metadata that is used to identify the node.
  I   short shTotalNodeSizeNeeded   Contains the size of the node that is searched for
  I   short shNodeType  Contains the type of node that is searched for
  I   SMResult *psmResult   Flag to notify program and user of discrepancies and failures
  0   FreeNode *pTemp   Pointer to node that meets criteria for resuse. 

Return Value:
  0   FreeNode *pTemp   Pointer to node that meets criteria for resuse. 
 
***************FreeNode***********************/
FreeNode *searchFreeListForNodes(StorageManager *pMgr, short shTotalNodeSizeNeeded, short shNodeType, SMResult *psmResult){

  //Temp ptr used to return if we find a node in the list
  FreeNode *pTemp = NULL;

  //Point to start of free list for given node type
  FreeNode *pTempFree = pMgr->pFreeHeadM[shNodeType];
  if(pTempFree == NULL) return NULL;


  //Check the first node outside of loop. 
  if(pTempFree->shAllocSize >= shTotalNodeSizeNeeded ){   
    //If found node of equal or greater size then assign our temp
    //node to it.
    pTemp = pTempFree;
    
    //Then splice the list 
    pTempFree = pTempFree->pNextFree;

    //Assign list ptr to new head of list
    pMgr->pFreeHeadM[shNodeType] = pTempFree;

    return pTemp;
  }
  
  //Use another pointer to point to the next node in the list
  //This is used incase we pull a node from inbetwixt nodes
  //and splice the list back together
  FreeNode *pTempFreeNext = NULL;

  //Check if there is a next node to look at if
  //first free node is not what we want
  //If there isnt then we return null
  if(pTempFree->pNextFree != NULL){
    pTempFreeNext = pTempFree->pNextFree;
  }
  else{
    return NULL;
  }

  //Iterate thru the list searchin for a node of equal or greater size
  for(; pTempFreeNext != NULL; pTempFreeNext = pTempFreeNext->pNextFree){
    
    if(pTempFreeNext->shAllocSize >= shTotalNodeSizeNeeded ){   
      //If found node of equal or greater size then assign our temp
      //node to it.
      pTemp = pTempFree;
      
      //Then splice the list 
      pTempFree->pNextFree = pTempFreeNext->pNextFree;

      return pTemp;
    }

    //Always keep the prior node pointed at just incase we need to splice
    pTempFree = pTempFree->pNextFree;
  }

  //If no node is found because the free list is empty
  //then return null 
  return NULL;
}

/***************userAssoc***********************
void *userAssoc(StorageManager *pMgr, void *pUserDataFrom, char szAttrName[], void *pUserDataTo, SMResult *psmResult)

Purpose:
  The function updates the pNextCust/pNextItem to ref another node. Then it calls another function
  to update the reference count for the refernced node. If the pNextCust/pNextItem field is not 
  empty when updating, then a function is called to derefernce the old node. 
Parameters:
  I   StorageManager *pMgr Contains node metadata that is used to identify the node.
  I   void *pUserDataFrom   pointer to the node that will do the referencing.
  I   void *pUserDataTo     pointer to the node that will be referenced.
  I   char szAttrName       string containing the attribute name used to check if inputted attribute
                            is a pointer.
  I   SMResult *psmResult   Flag to notify program and user of discrepancies and failures
 
***************userAssoc***********************/
void userAssoc(StorageManager *pMgr, void *pUserDataFrom, char szAttrName[], void *pUserDataTo, SMResult *psmResult){
  //The passed in pointer is pointing to the user data part of
  //the node. So we must move the pointer back 8 bytes to 
  //point to the meta part of the node. 
  AllocNode *pFromNode = (AllocNode*) ( (char*) pUserDataFrom - pMgr->shPrefixSize);
  AllocNode *pToNode = (AllocNode*) ((char*)pUserDataTo - pMgr->shPrefixSize);

  void *pUserFromNode = pUserDataFrom;      //Points to userdata of from node
  void *pUserToNode = pUserDataTo;          //Points ot userdata of to node
  void **ppUserDataFrom_pNext;        //pointer to the pointer that points to next Cust/Item node

  //will contain offset from sbUserData to the pointer field that points to next Cust/Item node
  short shOffset;     
  
  //Use the node type to get the attr of the node
  short iAttr;

  /**Check Node Type for Ref'ed Node**/
  //Get the node type for the ref'ed node
  short shToNodeType = pToNode->shNodeType;

  int found = 0;
  //szAttrName must be used to know if the attr(szAttrName) is a pointer type 
  for(iAttr = 0; iAttr < MAX_NODE_ATTR; iAttr++){
    //Check if the node's meta attr is a pointer type
    if((strcmp(pMgr->metaAttrM[iAttr].szAttrName, szAttrName) == 0) && (pMgr->metaAttrM[iAttr].cDataType == 'P')){
	found = 1;
	break;
    }
  }
	
  if(!found){
	psmResult->rc = RC_ASSOC_ATTR_NOT_PTR;
        return;
  }
  //Assign offset of pNext in UserData to var
  shOffset = (pMgr->metaAttrM[iAttr].shOffset);
  
  /**Check if from node is ref'ing another node**/

  //Point to where the pNext field would be in Userdata
  //*ppUserDataFrom_pNext = (void**)(((char*)pUserFromNode) + shOffset);
  char *pFrom_pNext = ((char*)pUserFromNode + shOffset);
  ppUserDataFrom_pNext = (void**)pFrom_pNext;
  //Check if pNext is NULL
  if(*ppUserDataFrom_pNext != NULL){
    //If pNext not NULL then we have to decrement that nodes ref count
    //this is because the node may have ref to another node and thus that 
    //node would need its ref decrented as well 
    //this func then may need to recursively call itself
    userRemoveRef(pMgr, *ppUserDataFrom_pNext, psmResult);
    //Check if we had any problems within userRemoveRef func
    if(psmResult->rc != 0) return ;

  }
  
  //Set userData pNext to the toNode
  *ppUserDataFrom_pNext = pUserDataTo;  

  //After pointing to the new node increment its ref count
  userAddRef(pMgr, *ppUserDataFrom_pNext, psmResult);

  //Check if we had any problems within userAddRef func
  if(psmResult->rc != 0) return ;
}

/***************userAllocate***********************
void * userAllocate(StorageManager *pMgr, short shUserDataSize, short shNodeType, char sbUserData[], SMResult *psmResult){

Purpose:
  This function will allocate a new node either by finding an unused node that is in the free
  node list or having the driver return one. Once a node is obtained then the values are 
  initiated with provide parameters.
Parameters:
  I   StorageManager *pMgr    Contains node metadata that is used to identify the node.
  I   short shUserDataSize    Contains the size of the user data portion of node.
  I   short shNodeType        Contains the type of the node.
  I   char sbUserData[]       Block that contains the user data 
  I   SMResult *psmResult   Flag to notify program and user of discrepancies and failures

Return Value:
  0   void *pUserDataReturn   Pointer to new allocated node. 
 
***************userAllocate***********************/
void * userAllocate(StorageManager *pMgr, short shUserDataSize, short shNodeType, char sbUserData[], SMResult *psmResult){
 
  FreeNode *pTemp;        //pointer that will be used to point to the new free node
  //pointer to the very front of the new node. Will be used to initiate metadata
  AllocNode *pTempAllocNode;    

  //Calc the size of the whole node
  //Should be size of user binary data plus the size of the prefix(meta)
  short shTotalSize = shUserDataSize + pMgr->shPrefixSize;

  //Search for a free node first; returns NULL if not found
  pTemp = searchFreeListForNodes(pMgr, shTotalSize, shNodeType, psmResult);

  //If a free node is not found then use driver util to get a node 
  if(pTemp == NULL){
    //The size to pass must be data size plus the meta struct
    //The driver util returns an AllocNode type 
    pTempAllocNode = utilAlloc(pMgr, shTotalSize);
  }
  else{   //If we did find a free node 
    //Point the temp Allocnode to the returned new node
    pTempAllocNode = (AllocNode *)pTemp;
  }
  //Setup the meta and copy the user data to the new node
  pTempAllocNode->shAllocSize = shTotalSize;
  pTempAllocNode->cAF = 'A';
  pTempAllocNode->shRefCount = 1;
  pTempAllocNode->shNodeType = shNodeType;

  //Copy user data binary to the user data portion of node
  //this is part of node that is to be pointed at and returned
  memcpy(pTempAllocNode->sbData, sbUserData, shUserDataSize);

  //Return a pointer that points to start of user data not the 
  //start of the actual node meta and all. 
  void *pUserDataReturn = (char*)pTempAllocNode + pMgr->shPrefixSize;

  return pUserDataReturn;
}

/***************userRemoveRef***********************
void userRemoveRef(StorageManager *pMgr, void *pUserData, SMResult *psmResult){

Purpose:
  This function will allocate a new node either by finding an unused node that is in the free
  node list or having the driver return one. Once a node is obtained then the values are 
  initiated with provide parameters.
Parameters:
  I   StorageManager *pMgr    Contains node metadata that is used to identify the node.
  I   void *pUserData         Pointer to the node that needs its reference value decremented.
  I   SMResult *psmResult   Flag to notify program and user of discrepancies and failures.

***************userRemoveRef***********************/
void userRemoveRef(StorageManager *pMgr, void *pUserData, SMResult *psmResult){
  //In func pointer should point to front of node where meta starts
  AllocNode *pRefedNode = (AllocNode*)((char*)pUserData - pMgr->shPrefixSize);
  //Temp pointer that points to the userdata of node that is to be derefed
  void *pTempUserData = pUserData;
  
  //Pointer to the pNextCust field of this node
  void **pUserData_pNext;
  //Variable to hold the offset to the pNext pointer field
  short shOffsetTo_pNext = 0;

  //Get the node type for the ref'ed node
  short shToNodeType = pRefedNode->shNodeType;
  int found = 0;  
  //Look for the offset for pNextCust or if LineItem then look for its offset
  //shBeginMetaAttr gives us the offset based on node type
  for(int iAttr = pMgr->nodeTypeM[shToNodeType].shBeginMetaAttr; iAttr < MAX_NODE_ATTR; iAttr++){
    //if we find the attribute names pNextCust/pNextItem then assign the offset for that node 
    //type to shOffsetTo_pNext
    if((strcmp(pMgr->metaAttrM[iAttr].szAttrName, "pNextCust") ==0) || (strcmp(pMgr->metaAttrM[iAttr].szAttrName, "pNextItem") ==0)){
      shOffsetTo_pNext = pMgr->metaAttrM[iAttr].shOffset;
      found = 1;
      break;
    }
  }

  if(!found){
    psmResult->rc = NOT_FOUND;
    return;
  }

  //Pointer to the pNext of this node
  pUserData_pNext = (void**)(((char*)pTempUserData) + shOffsetTo_pNext);
  
  //decrement ref count by one for this node
  pRefedNode->shRefCount--;

  //If ref count drops to zero then it must be freed.
  //But it must be checked for any nodes it may ref
  //So recursively call(done in the func itself) this func and decrement that nodes ref count
  if(pRefedNode->shRefCount <= 0){
    if(*pUserData_pNext != NULL){   //check for any nodes this node has ref's to 
      //If it does have ref's to other nodes then we decrement its ref count
      userRemoveRef(pMgr, *pUserData_pNext, psmResult);
    }
    //NULL found, node has ref count of zero and it has no ref's to other nodes, free it!
    memFree(pMgr, pRefedNode, psmResult);
  }

  //Ref count of node is not zero so just return after the decrement.
  return;
}

/***************userAddRef***********************
void userAddRef(StorageManager *pMgr, void *pUserDataTo, SMResult *psmResult){

Purpose:
  This function will increment a nodes reference count by one.
Parameters:
  I   StorageManager *pMgr    Contains node metadata that is used to identify the node.
  I   void *pUserDataTo       Pointer to the node that needs its reference value incremented.
  I   SMResult *psmResult     Flag to notify program and user of discrepancies and failures.

***************userAddRef***********************/
void userAddRef(StorageManager *pMgr, void *pUserDataTo, SMResult *psmResult){
  AllocNode *pRefedNode = (AllocNode*)((char*)pUserDataTo - pMgr->shPrefixSize);
  pRefedNode->shRefCount++;
}

/***************memFree***********************
void memFree(StorageManager *pMgr, AllocNode *pAlloc, SMResult *psmResult){

Purpose:
  This function will increment a nodes reference count by one.
Parameters:
  I   StorageManager *pMgr    Contains node metadata that is used to identify the node.
  I   Alloc *pAlloc           Point to the node that will no longer be used and be put in the 
                              free node list based on type.
  I   SMResult *psmResult     Flag to notify program and user of discrepancies and failures.

***************memFree***********************/
void memFree(StorageManager *pMgr, AllocNode *pAlloc, SMResult *psmResult){
  AllocNode *pTempNode = pAlloc;
  if(pTempNode->cAF == 'A'){
    //Get the node type for the node to be freed
    short shAllocNodeType = pTempNode->shNodeType;
    
    //The size of the node to be added to the free list
    short shSizeOfNode = pTempNode->shAllocSize;

    //Temp pointer to the head of the free list for specified node type
    FreeNode *pTempHeadFreeNode = pMgr->pFreeHeadM[shAllocNodeType];
    
    //Temp FreeNode pointer to the node that is to be added to the free list
    FreeNode *pTempNewFreeNode = (FreeNode*)pTempNode; 
    pTempNewFreeNode->cAF = 'F';
    pTempNewFreeNode->shAllocSize = shSizeOfNode;

    //Point head of free list to the new node
    pMgr->pFreeHeadM[shAllocNodeType] = pTempNewFreeNode;
 
    //If old head is not NULL then point new head to it else make pNextFree = NULL
    if(pTempHeadFreeNode == NULL){
      pMgr->pFreeHeadM[shAllocNodeType]->pNextFree = NULL;
      return;
    }
    else{
      //Point the new head of free list to rest of the list
      pMgr->pFreeHeadM[shAllocNodeType]->pNextFree = pTempHeadFreeNode;
      return;
    }
  }
  else{
    psmResult->rc = RC_CANT_FREE; 
    return;
  }

}
