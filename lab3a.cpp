//NAME: TANMAYA HADA
//EMAIL: tanmaya2000@hotmail.com
//ID:304925920
#include <stdio.h>
#include <iomanip>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include "ext2_fs.h"
#include <iostream>

using namespace std;

int difd=-1;

void indirect_recursive(int parentInode, const struct ext2_inode &inode, int lev, int blockNumber, int logOff, int blockSize);

int main(int argc,char** argv){
  difd=open(argv[1],O_RDONLY);
  if(argc != 2){
    fprintf(stderr, "Error: bad argument!!!\n");
    exit(1);
  }
  if(difd<0){
    fprintf(stderr,"Error: bad argument!!!\n");
    exit(1);
  }
  
  struct ext2_super_block supBlock;
  
  int numBytes;
  numBytes= pread(difd,&supBlock,sizeof(struct ext2_super_block),1024);
  
  if(numBytes < 0){
    fprintf(stderr,"pread failed: %s!\n",strerror(errno));
    exit(2); 
  }
  
  if(numBytes < (int)sizeof(struct ext2_super_block)){
    fprintf(stderr,"pread failed: Not enough bytes read!\n");
    exit(2);
  }
  
  int blockSize = 1024 << supBlock.s_log_block_size;
  fprintf(stdout,"SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",supBlock.s_blocks_count,
	  supBlock.s_inodes_count,
	  blockSize,
	  supBlock.s_inode_size,
	  supBlock.s_blocks_per_group,
	  supBlock.s_inodes_per_group,
	  supBlock.s_first_ino);
  
  int grpNo = supBlock.s_block_group_nr;
  struct ext2_group_desc group;
  
  numBytes= pread(difd,&group,sizeof(struct ext2_group_desc),1024+sizeof(struct ext2_super_block));
  if(numBytes < 0){
    fprintf(stderr,"pread failed: %s!\n",strerror(errno));
    exit(2); 
  }
  if(numBytes < (int)sizeof(struct ext2_group_desc)){
    fprintf(stderr,"pread failed: Not enough bytes read!\n");
    exit(2);
  }
  fprintf(stdout,"GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",grpNo,
	  supBlock.s_blocks_count,
	  supBlock.s_inodes_count,
	  group.bg_free_blocks_count,
	  group.bg_free_inodes_count,
	  group.bg_block_bitmap,
	  group.bg_inode_bitmap,
	  group.bg_inode_table);
 
  int countByte=0;
  unsigned int i;
  unsigned char buf;
  int bits = 0;
  numBytes= pread(difd,&buf,1,blockSize*group.bg_block_bitmap);
  if(numBytes < 0){
    fprintf(stderr,"pread failed: %s!\n",strerror(errno));
    exit(2); 
  }
  if(numBytes < 1){
    fprintf(stderr,"pread failed: Not enough bytes read!\n");
    exit(2);
  }
  for(i =0; i<supBlock.s_blocks_count;i++){
    
    if(((buf >> bits) & 1) == 0){
      fprintf(stdout,"BFREE,%d\n",i+1);
    }
    bits++;
    if(bits==8){
      bits = 0;
      countByte++;
      numBytes= pread(difd,&buf,1,blockSize*group.bg_block_bitmap+countByte);
      if(numBytes < 0){
	fprintf(stderr,"pread failed: %s!\n",strerror(errno));
	exit(2); 
      }
      if(numBytes < 1){
	fprintf(stderr,"pread failed: Not enough bytes read!\n");
	exit(2);
      }
    }
  }
  
   bits = 0;
  countByte=0;
  numBytes= pread(difd,&buf,1,blockSize*group.bg_inode_bitmap);
  if(numBytes < 0){
    fprintf(stderr,"pread failed: %s!\n",strerror(errno));
    exit(2); 
  }
  if(numBytes < 1){
    fprintf(stderr,"pread failed: Not enough bytes read!\n");
    exit(2);
  }
   for(i =0; i<supBlock.s_inodes_count;i++){
    
     if(((buf >> bits) & 1) == 0){
      fprintf(stdout,"IFREE,%d\n",i+1);
    }
    bits++;
    if(bits==8){
      bits = 0;
      countByte++;
      numBytes= pread(difd,&buf,1,blockSize*group.bg_inode_bitmap+countByte);
      if(numBytes < 0){
	fprintf(stderr,"pread failed: %s!\n",strerror(errno));
	exit(2); 
      }
      if(numBytes < 1){
	fprintf(stderr,"pread failed: Not enough bytes read!\n");
	exit(2);
      }
    }
    }
  
  for(i=0; i<supBlock.s_inodes_count;i++){
    struct ext2_inode inode;
    numBytes= pread(difd,&inode,sizeof(struct ext2_inode),blockSize*group.bg_inode_table+i*sizeof(struct ext2_inode));
    if(numBytes < 0){
	fprintf(stderr,"pread failed: %s!\n",strerror(errno));
	exit(2); 
    }
    if(numBytes < (int)sizeof(struct ext2_inode)){
	fprintf(stderr,"pread failed: Not enough bytes read!\n");
	exit(2);
    }
    __u16 ftype = inode.i_mode & 0xF000;
    __u16 mode = inode.i_mode & 0x0FFF;
    char file;
    switch(ftype){
    case 0xA000:
      file='s';
      break;
    case 0x8000:
      file='f';
      break;
    case 0x4000:
      file ='d';
      break;
    default:
      file ='?';
      break;
    }
    
    if(!(mode != 0 && inode.i_links_count != 0)){
      continue;
    }
    
    const time_t change = inode.i_ctime;
    const time_t mod = inode.i_mtime;
    const time_t acc = inode.i_atime;
    const struct tm ctime = *gmtime(&change);
    const struct tm mtime = *gmtime(&mod);
    const struct tm atime = *gmtime(&acc);
    char octime[20],oatime[20], omtime[20];
    strftime(octime,20,"%m/%d/%y %H:%M:%S", &ctime);
    strftime(omtime,20,"%m/%d/%y %H:%M:%S", &mtime);
    strftime(oatime,20,"%m/%d/%y %H:%M:%S", &atime);
    fprintf(stdout,"INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",i+1,
	    file,
	    mode,
	    inode.i_uid,
	    inode.i_gid,
	    inode.i_links_count,
	    octime,
	    omtime,
	    oatime,
	    inode.i_size,
	    inode.i_blocks);

    if(!(file == 's' && inode.i_size < 60)){
      int j;
      for(j=0;j<15;j++){
	fprintf(stdout,",%d",inode.i_block[j]);
      }
    }
    fprintf(stdout,"\n");  

    if(file == 'd'){
      string dstr ="";
      int blocks = inode.i_blocks / (blockSize/512);
      int j;
      int done =0;
      int count=0;
      for(j=0; j<12;j++){
	if(count==blocks){
	  done =1;
	  break;
	}
	if(inode.i_block[j]==0){
	  count++;
	  continue;
	}
        char* buff = new char[blockSize];
      	numBytes= pread(difd,buff,blockSize,blockSize*inode.i_block[j]);
	if(numBytes < 0){
	  fprintf(stderr,"pread failed: %s!\n",strerror(errno));
	  exit(2); 
	}
	if(numBytes < blockSize){
	  fprintf(stderr,"pread failed: Not enough bytes read!\n");
	  exit(2);
	}
	dstr.append(buff,blockSize);
	delete[] buff;
	count++;
      }
      
      if(!done){
	__u32* indBlock = new __u32[blockSize/4];
	numBytes= pread(difd,indBlock,blockSize,blockSize*inode.i_block[12]);
	if(numBytes < 0){
	  fprintf(stderr,"pread failed: %s!\n",strerror(errno));
	  exit(2); 
	}
	if(numBytes < blockSize){
	  fprintf(stderr,"pread failed: Not enough bytes read!\n");
	  exit(2);
	}
	for(j=0;j<blockSize/4;j++){
	  if(count == blocks){
	    done  = 1;
	    break;
	  }
	  if(indBlock[j]==0){
	    count++;
	    continue;
	  }
	  char* buff= new char[blockSize];
	  numBytes= pread(difd,buff,blockSize,blockSize*indBlock[j]);
	  if(numBytes < 0){
	    fprintf(stderr,"pread failed: %s!\n",strerror(errno));
	    exit(2); 
	  }
	  if(numBytes < blockSize){
	    fprintf(stderr,"pread failed: Not enough bytes read!\n");
	    exit(2);
	  }
	  dstr.append(buff,blockSize);
	  delete[] buff;
	  count++;
	  
	}
	delete[] indBlock;
      }
      
      if(!done){
	__u32* ind2Block = new __u32[blockSize/4];
	numBytes= pread(difd,ind2Block,blockSize,blockSize*inode.i_block[13]);
	if(numBytes < 0){
	  fprintf(stderr,"pread failed: %s!\n",strerror(errno));
	  exit(2); 
	}
	if(numBytes < blockSize){
	  fprintf(stderr,"pread failed: Not enough bytes read!\n");
	  exit(2);
	}
	for(j=0;j<blockSize/4;j++){
	  __u32* indBlock = new __u32[blockSize/4];
	  numBytes= pread(difd,indBlock,blockSize,blockSize*ind2Block[j]);
	  if(numBytes < 0){
	    fprintf(stderr,"pread failed: %s!\n",strerror(errno));
	    exit(2); 
	  }
	  if(numBytes < blockSize){
	    fprintf(stderr,"pread failed: Not enough bytes read!\n");
	    exit(2);
	  }
	  int k=0;
	  for(k=0;k<blockSize/4;k++){
	    if(count == blocks){
	      done  = 1;
	      break;
	    }
	    if(indBlock[k]==0){
	      count++;
	      continue;
	    }
	    char* buff=new char[blockSize];
	    numBytes= pread(difd,buff,blockSize,blockSize*indBlock[k]);
	    if(numBytes < 0){
	      fprintf(stderr,"pread failed: %s!\n",strerror(errno));
	      exit(2); 
	    }
	    if(numBytes < blockSize){
	      fprintf(stderr,"pread failed: Not enough bytes read!\n");
	      exit(2);
	    }
	    dstr.append(buff,blockSize);
	    delete[] buff;
	    count++;
	  
	  }
	  delete[] indBlock;
	  if(done)
	    break;
	}
	delete[] ind2Block;
      }
      
      if(!done){
	__u32* ind3Block = new __u32[blockSize/4];
	numBytes= pread(difd,ind3Block,blockSize,blockSize*inode.i_block[14]);
	if(numBytes < 0){
	  fprintf(stderr,"pread failed: %s!\n",strerror(errno));
	  exit(2); 
	}
	if(numBytes < blockSize){
	  fprintf(stderr,"pread failed: Not enough bytes read!\n");
	  exit(2);
	}
	
	for(j=0;j<blockSize/4;j++){
	  __u32* ind2Block = new __u32[blockSize/4];
	  numBytes= pread(difd,ind2Block,blockSize,blockSize*ind3Block[j]);
	  if(numBytes < 0){
	    fprintf(stderr,"pread failed: %s!\n",strerror(errno));
	    exit(2); 
	  }
	  if(numBytes < blockSize){
	    fprintf(stderr,"pread failed: Not enough bytes read!\n");
	    exit(2);
	  }
	  int k=0;
	  
	  for(k=0;k<blockSize/4;k++){
	    __u32* indBlock = new __u32[blockSize/4];
	    numBytes= pread(difd,indBlock,blockSize,blockSize*ind2Block[k]);
	    if(numBytes < 0){
	      fprintf(stderr,"pread failed: %s!\n",strerror(errno));
	      exit(2); 
	    }
	    if(numBytes < blockSize){
	      fprintf(stderr,"pread failed: Not enough bytes read!\n");
	      exit(2);
	    }
	    int m;
	    
	    for(m=0;m<blockSize/4;m++){
	      if(count == blocks){
		done  = 1;
		break;
	      }
	      if(indBlock[m]==0){
		count++;
		continue;
	      }
	      char* buff= new char[blockSize];
	      numBytes= pread(difd,buff,blockSize,blockSize*indBlock[m]);
	      if(numBytes < 0){
		fprintf(stderr,"pread failed: %s!\n",strerror(errno));
		exit(2); 
	      }
	      if(numBytes < blockSize){
		fprintf(stderr,"pread failed: Not enough bytes read!\n");
		exit(2);
	      }
	      dstr.append(buff, blockSize);
	      delete[] buff;
	      count++;
	      
	    }
	  delete[] indBlock;
	  if(done)
	    break;
	  }
	delete[] ind2Block;
	if(done)
	  break;
	}
	delete[] ind3Block;
      }
      
      const char* data = dstr.c_str();
      unsigned int off=0;
      while(off < inode.i_size){
	struct ext2_dir_entry dentry;
	memcpy(&dentry, data+off, sizeof(struct ext2_dir_entry));
	if(dentry.inode == 0){
	  off += dentry.rec_len;
	  continue;
	}
	fprintf(stdout,"DIRENT,%d,%d,%d,%d,%d,'%s'\n",i+1
		,off
		,dentry.inode
		,dentry.rec_len
		,dentry.name_len
		,dentry.name);
	off += dentry.rec_len;
      }
    }
    indirect_recursive(i,inode,1,inode.i_block[12],12, blockSize);
    indirect_recursive(i,inode,2,inode.i_block[13],12+blockSize/4, blockSize);
    indirect_recursive(i,inode,3,inode.i_block[14],12+(blockSize/4)+(blockSize/4)*(blockSize/4),blockSize);
  }
}

void indirect_recursive(int parentInode, const struct ext2_inode &inode, int lev, int blockNumber, int logOff, int blockSize){
  if(blockNumber == 0)
    return;
  __u32* arrPtr = new __u32[blockSize/4];
  int numBytes;
  numBytes= pread(difd,arrPtr,blockSize,blockSize*blockNumber);
  if(numBytes < 0){
    fprintf(stderr,"pread failed: %s!\n",strerror(errno));
    exit(2); 
  }
  if(numBytes < blockSize){
    fprintf(stderr,"pread failed: Not enough bytes read!\n");
    exit(2);
  }
  int i;
  for(i=0;i<blockSize/4;i++){
    if(arrPtr[i] == 0)
      continue;
    fprintf(stdout,"INDIRECT,%d,%d,%d,%d,%d\n",parentInode+1, lev, logOff+i, blockNumber, arrPtr[i]);
    if(lev>1)
      indirect_recursive(parentInode,inode,lev-1,arrPtr[i],logOff+i,blockSize);
  }
  delete[] arrPtr;
}


