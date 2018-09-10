#include "cs3723p1.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

void userAssoc(StorageManager *pMgr, void *pUserDataFrom, char szAttrName[], void *pUserDataTo, SMResult *psmResult){
  //The passed in pointer is pointing to the user data part of
  //the node. So we must move the pointer back 8 bytes to 
  //point to the meta part of the node. 
  AllocNode *pFromNode = (AllocNode*) ( (char*) pUserDataFrom - pMgr->shPrefixSize);
  AllocNode *pToNode = (AllocNode*) ((char*)pUserDataTo - pMgr->shPrefixSize);

  void *pUserFromNode = pUserDataFrom;      //Points to userdata of from node
  void *pUserToNode = pUserDataTo;          //Points ot userdata of to node
  AllocNode *pRefedNode;
  void **pUserData_pNext;

  short shFromNodepNext = 0;
  short shOffset;

  /**Check Node Type for Ref'ed Node**/

  //Get the node type for the ref'ed node
  short shToNodeType = pToNode->shNodeType;

  //Use the node type to get the attr of the node
  short iAttr = pMgr->nodeTypeM[shToNodeType].shBeginMetaAttr;
  
  //Check if the node's meta attr is a pointer type
  if(pMgr->metaAttrM[iAttr].cDataType != 'P'){
    psmResult->rc = RC_ASSOC_ATTR_NOT_PTR;
    return;
  }

  //Assign offset of pNext in UserData to var
  shOffset = (pMgr->metaAttrM[iAttr].shOffset);
  
  
  /**Check if from node is ref'ing another node**/

  //Point to where the pNext field would be in Userdata
  pUserData_pNext = (void*)(((char*)pUserFromNode) + shOffset);
  //Check if pNext is NULL
  if(*pUserData_pNext != NULL){
    //If not NULL then we have to decrement that nodes ref count

    /****use the userRemoveRef func here****/
    //this is becuase the node may have ref to another node and thus that 
    //node would need its ref decrented as well 
    //this func then may need to recursively call itself
    userRemoveRef(pMgr, *pUserData_pNext, psmResult);

  }
  
  //Set userData pNext to the toNode
  *pUserData_pNext = pUserDataTo;  

  //After pointing to the new node increment its ref count
  userAddRef(pMgr, *pUserData_pNext, psmResult);
  //pRefedNode = (AllocNode*)((char*)pUserDataTo - pMgr->shPrefixSize);
  //pRefedNode->shRefCount++;

  psmResult->rc = 0;


}

void * userAllocate(StorageManager *pMgr, short shUserDataSize, short shNodeType, char sbUserData[], SMResult *psmResult){
 
  FreeNode *pTemp;
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


void userRemoveRef(StorageManager *pMgr, void *pUserData, SMResult *psmResult){

}

void userAddRef(StorageManager *pMgr, void *pUserDataTo, SMResult *psmResult){

}

void memFree(StorageManager *pMgr, AllocNode *pAlloc, SMResult *psmResult){


}