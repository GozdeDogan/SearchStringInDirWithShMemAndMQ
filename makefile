all: 
	  clear
	  gcc 131044019_Gozde_Dogan_HW5.c restart.c -o grepSh -lpthread
clear:
	  rm *.o grepSh
