pipeline {
    agent { dockerfile true }
    environment {
        CC='gcc'
        CFLAGS='-Werror'
        ARTIFACT_DIR='artifacts'
        TEST_DIR='tests'
        FUZZ_DIR='fuzz'
        FAND_STEM='amdgpu-fand-0.4'
        FANCTL_STEM='amdgpu-fanctl-0.4'
        TEST_STEM='amdgpu-fand-0.4-test'
        FUZZ_STEM='amdgpu-fuzzd-0.4'
    }
    stages {
        stage('Gitlab Pending') {
            steps {
                echo 'Notifying Gitlab'
                updateGitlabCommitStatus name: 'build', state: 'pending'
            }
        }
        stage('Doc') {
            steps {
                echo '-- Building docs -- '
                sh 'make doc'
            }
        }
        stage('Build') {
            steps {
                echo '-- Starting build --'

                echo 'Creating artifact directory'
                sh 'mkdir -p ${ARTIFACT_DIR}'

                echo 'Build: CC=gcc DRM=y'
                sh '''
                    export CC=gcc
                    export FAND=${ARTIFACT_DIR}/${FAND_STEM}.${BUILD_NUMBER}-drm-${CC}
                    export FANCTL=${ARTIFACT_DIR}/${FANCTL_STEM}.${BUILD_NUMBER}-drm-${CC}
                    make -j$(nproc) -B
                '''

                echo 'Build: CC=clang DRM=y'
                sh '''
                    export CC=clang
                    export FAND=${ARTIFACT_DIR}/${FAND_STEM}.${BUILD_NUMBER}-drm-${CC}
                    export FANCTL=${ARTIFACT_DIR}/${FANCTL_STEM}.${BUILD_NUMBER}-drm-${CC}
                    make -j$(nproc) -B
                '''

                echo 'Build: CC=gcc DRM=n'
                sh '''
                    export CC=gcc
                    export FAND=${ARTIFACT_DIR}/${FAND_STEM}.${BUILD_NUMBER}-${CC}
                    export FANCTL=${ARTIFACT_DIR}/${FANCTL_STEM}.${BUILD_NUMBER}-${CC}
                    make drm_support=n -j$(nproc) -B
                '''

                echo 'Build: CC=clang DRM=n'
                sh '''
                    export CC=clang
                    export FAND=${ARTIFACT_DIR}/${FAND_STEM}.${BUILD_NUMBER}-${CC}
                    export FANCTL=${ARTIFACT_DIR}/${FANCTL_STEM}.${BUILD_NUMBER}-${CC}
                    make drm_support=n -j$(nproc) -B
                '''

                echo 'Creating test directory'
                sh 'mkdir -p ${TEST_DIR}'

                echo 'Build: CC=gcc DRM=y test'
                sh '''
                    export CC=gcc
                    export FAND_TEST=${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-drm-${CC}
                    make test -j$(nproc) -B
                '''

                echo 'Build: CC=clang DRM=y test'
                sh '''
                    export CC=clang
                    export FAND_TEST=${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-drm-${CC}
                    make test -j$(nproc) -B
                '''

                echo 'Build: CC=gcc DRM=n test'
                sh '''
                    export CC=gcc
                    export FAND_TEST=${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-${CC}
                    make test drm_support=n -j$(nproc) -B
                '''

                echo 'Build: CC=clang DRM=n test'
                sh '''
                    export CC=clang
                    export FAND_TEST=${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-${CC}
                    make test drm_support=n -j$(nproc) -B
                '''

                echo 'Creating fuzz directory'
                sh 'mkdir -p ${FUZZ_DIR}'

                echo 'Build: CC=clang fuzz'
                sh '''
                    export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-clang
                    make fuzz -j$(nproc) -B
                '''
            }
        }
        stage('Test') {
            steps {
                echo '-- Starting tests --'

                echo 'Test: CC=gcc DRM=y'
                sh '${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-drm-gcc'

                echo 'Test: CC=clang DRM=y'
                sh '${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-drm-clang'

                echo 'Test: CC=gcc DRM=n'
                sh '${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-gcc'

                echo 'Test: CC=clang DRM=n'
                sh '${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-clang'
            }
        }
        stage('Fuzz') {
            steps {
                echo '-- Starting fuzzing --'
                sh 'make fuzzrun FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-clang'
            }
        }
        stage('Gitlab Success') {
            steps {
                echo 'Notifying Gitlab'
                updateGitlabCommitStatus name: 'build', state: 'success'
            }
        }
    }
    post {
        success {
            echo 'No errors encountered'
        }
        always {
            echo 'Cleaning up'
            deleteDir()
        }
    }
}

