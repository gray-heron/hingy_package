default:
	make -C hingybot
	make -C evolutionary_trainer
	cd TORCSTester && msbuild

clean:
	make clean -C hingybot
	make clean -C evolutionary_trainer
	cd TORCSTester && msbuild /t:Clean
