pipeline {
    agent { label 'jenkinsfile' }

    stages {
        stage('Preamble') {
            parallel {
                stage('x86_64 linux clang-18') {
            	     agent { label 'x86_64 && ubuntu24' }
                     options { skipDefaultCheckout() }
            	     steps {
                         cleanWs()
                         checkout scm
	    	     	 sh '''
                	 echo "x86_64 clang-18 on" `hostname`
			 export CC=clang-18
			 export CXX=clang++-18
 			 mkdir build
			 cd build
			 cmake -GNinja -DCMAKE_INSTALL_PREFIX=../../install -DENABLE_ASAN=True ..
			 cmake -E time ninja
		         export CTEST_OUTPUT_ON_FAILURE=TRUE
		         ctest -j `nproc`
			 '''
            	     }
                }

                stage('x86_64 linux gcc-13') {
            	     agent { label 'x86_64 && ubuntu24' }
                     options { skipDefaultCheckout() }
            	     steps {
                         cleanWs()
                         checkout scm
	    	     	 sh '''
                	 echo "x86_64 gcc-13 on" `hostname`
			 export CC=gcc-13
			 export CXX=g++-13
 			 mkdir build
			 cd build
			 cmake -GNinja -DCMAKE_INSTALL_PREFIX=../../install -DENABLE_ASAN=True ..
			 cmake -E time ninja
		         export CTEST_OUTPUT_ON_FAILURE=TRUE
		         ctest -j `nproc`
			 '''
            	     }
                }

                stage('aarch64 linux gcc-14') {
            	     agent { label 'aarch64 && ubuntu24' }
                     options { skipDefaultCheckout() }
            	     steps {
                         cleanWs()
                         checkout scm
	    	     	 sh '''
                	 echo "aarch64 gcc-14 on" `hostname`
			 export CC=gcc-14
			 export CXX=g++-14
 			 mkdir build
			 cd build
			 cmake -GNinja -DCMAKE_INSTALL_PREFIX=../../install -DENABLE_ASAN=True ..
			 cmake -E time oomstaller ninja -j `nproc`
		         export CTEST_OUTPUT_ON_FAILURE=TRUE
		         ctest -j `nproc`
			 '''
            	     }
                }

                stage('aarch64 linux clang-19') {
            	     agent { label 'aarch64 && ubuntu24' }
                     options { skipDefaultCheckout() }
            	     steps {
                         cleanWs()
                         checkout scm
	    	     	 sh '''
                	 echo "aarch64 clang-19 on" `hostname`
			 export CC=clang-19
			 export CXX=clang++-19
 			 mkdir build
			 cd build
			 cmake -GNinja -DCMAKE_INSTALL_PREFIX=../../install -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld-19" -DENABLE_ASAN=True ..
			 cmake -E time oomstaller ninja -j `nproc`
		         export CTEST_OUTPUT_ON_FAILURE=TRUE
		         ctest -j `nproc`
			 '''
            	     }
                }

                stage('x86_64 windows clang') {
            	     agent { label 'windows11 && vs2022' }
                     options { skipDefaultCheckout() }
            	     steps {
                         cleanWs()
                         checkout scm
		     	 bat """
			 call "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat"
			 if not %ERRORLEVEL% == 0 exit /b %ERRORLEVEL%
			 call "winbuild-clang.bat" -DCMAKE_BUILD_TYPE=Release
			 if not %ERRORLEVEL% == 0 exit /b %ERRORLEVEL%
			 ctest -j 4 --output-on-failure
			 exit /b %ERRORLEVEL%
			 """
		     }
		}
            }
        }
    }
}
