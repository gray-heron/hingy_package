default:
	make -C hingybot
	make -C evolutionary_trainer
	cd TORCSTester && xbuild

clean:
	make clean -C hingybot
	make clean -C evolutionary_trainer
	cd TORCSTester && xbuild /t:Clean
