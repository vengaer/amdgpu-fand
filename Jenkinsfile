pipeline {
    agent none
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
        MUSL_DOCKER_IMAGE='fand/musl'
        GLIBC_DOCKER_IMAGE='fand/glibc'
    }
    stages {
        stage('Gitlab Pending') {
            steps {
                echo 'Notifying Gitlab'
                updateGitlabCommitStatus name: 'build', state: 'pending'
            }
        }
        stage('Docker Images') {
            failFast true
            parallel {
                stage('Build Musl Docker Image') {
                    agent any
                    steps {
                        echo '-- Docker musl image --'
                        sh '''
                            docker build -f docker/musl/Dockerfile -t ${MUSL_DOCKER_IMAGE} .
                        '''
                    }
                }
                stage('Build Glibc Docker Image') {
                    agent any
                    steps {
                        echo '-- Docker glibc image --'
                        sh '''
                            docker build -f docker/glibc/Dockerfile -t ${GLIBC_DOCKER_IMAGE} .
                        '''
                    }
                }
            }
        }
        stage('Doc') {
            agent {
                docker { image "${MUSL_DOCKER_IMAGE}" }
            }
            steps {
                echo '-- Building docs -- '
                sh 'make doc -B'
            }
        }
        stage('Build') {
            failFast true
            parallel {
                stage('Build Musl') {
                    agent {
                        docker { image "${MUSL_DOCKER_IMAGE}" }
                    }
                    environment {
                        LIBC='musl'
                        BUILDDIR='build_musl'
                    }
                    steps {
                        echo '-- Starting musl build --'

                        echo 'Creating artifact directory'
                        sh 'mkdir -p ${ARTIFACT_DIR}'

                        echo 'Build: CC=gcc DRM=y'
                        sh '''
                            export CC=gcc
                            export FAND=${ARTIFACT_DIR}/${FAND_STEM}.${BUILD_NUMBER}-drm-${CC}-${LIBC}
                            export FANCTL=${ARTIFACT_DIR}/${FANCTL_STEM}.${BUILD_NUMBER}-drm-${CC}-${LIBC}
                            make -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Build: CC=clang DRM=y'
                        sh '''
                            export CC=clang
                            export FAND=${ARTIFACT_DIR}/${FAND_STEM}.${BUILD_NUMBER}-drm-${CC}-${LIBC}
                            export FANCTL=${ARTIFACT_DIR}/${FANCTL_STEM}.${BUILD_NUMBER}-drm-${CC}-${LIBC}
                            make -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Build: CC=gcc DRM=n'
                        sh '''
                            export CC=gcc
                            export FAND=${ARTIFACT_DIR}/${FAND_STEM}.${BUILD_NUMBER}-${CC}-${LIBC}
                            export FANCTL=${ARTIFACT_DIR}/${FANCTL_STEM}.${BUILD_NUMBER}-${CC}-${LIBC}
                            make drm_support=n -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Build: CC=clang DRM=n'
                        sh '''
                            export CC=clang
                            export FAND=${ARTIFACT_DIR}/${FAND_STEM}.${BUILD_NUMBER}-${CC}-${LIBC}
                            export FANCTL=${ARTIFACT_DIR}/${FANCTL_STEM}.${BUILD_NUMBER}-${CC}-${LIBC}
                            make drm_support=n -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Creating test directory'
                        sh 'mkdir -p ${TEST_DIR}'

                        echo 'Build: CC=gcc DRM=y test'
                        sh '''
                            export CC=gcc
                            export FAND_TEST=${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-drm-${CC}-${LIBC}
                            make test -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Build: CC=clang DRM=y test'
                        sh '''
                            export CC=clang
                            export FAND_TEST=${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-drm-${CC}-${LIBC}
                            make test -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Build: CC=gcc DRM=n test'
                        sh '''
                            export CC=gcc
                            export FAND_TEST=${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-${CC}-${LIBC}
                            make test drm_support=n -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Build: CC=clang DRM=n test'
                        sh '''
                            export CC=clang
                            export FAND_TEST=${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-${CC}-${LIBC}
                            make test drm_support=n -j$(nproc) -B builddir=${BUILDDIR}
                        '''
                    }
                }
                stage('Build Glibc') {
                    agent {
                        docker { image "${GLIBC_DOCKER_IMAGE}" }
                    }
                    environment {
                        LIBC='glibc'
                        BUILDDIR='build_glibc'
                    }
                    steps {
                        echo '-- Starting glibc build --'

                        echo 'Creating artifact directory'
                        sh 'mkdir -p ${ARTIFACT_DIR}'

                        echo 'Build: CC=gcc DRM=y'
                        sh '''
                            export CC=gcc
                            export FAND=${ARTIFACT_DIR}/${FAND_STEM}.${BUILD_NUMBER}-drm-${CC}-${LIBC}
                            export FANCTL=${ARTIFACT_DIR}/${FANCTL_STEM}.${BUILD_NUMBER}-drm-${CC}-${LIBC}
                            make -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Build: CC=clang DRM=y'
                        sh '''
                            export CC=clang
                            export FAND=${ARTIFACT_DIR}/${FAND_STEM}.${BUILD_NUMBER}-drm-${CC}-${LIBC}
                            export FANCTL=${ARTIFACT_DIR}/${FANCTL_STEM}.${BUILD_NUMBER}-drm-${CC}-${LIBC}
                            make -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Build: CC=gcc DRM=n'
                        sh '''
                            export CC=gcc
                            export FAND=${ARTIFACT_DIR}/${FAND_STEM}.${BUILD_NUMBER}-${CC}-${LIBC}
                            export FANCTL=${ARTIFACT_DIR}/${FANCTL_STEM}.${BUILD_NUMBER}-${CC}-${LIBC}
                            make drm_support=n -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Build: CC=clang DRM=n'
                        sh '''
                            export CC=clang
                            export FAND=${ARTIFACT_DIR}/${FAND_STEM}.${BUILD_NUMBER}-${CC}-${LIBC}
                            export FANCTL=${ARTIFACT_DIR}/${FANCTL_STEM}.${BUILD_NUMBER}-${CC}-${LIBC}
                            make drm_support=n -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Creating test directory'
                        sh 'mkdir -p ${TEST_DIR}'

                        echo 'Build: CC=gcc DRM=y test'
                        sh '''
                            export CC=gcc
                            export FAND_TEST=${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-drm-${CC}-${LIBC}
                            make test -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Build: CC=clang DRM=y test'
                        sh '''
                            export CC=clang
                            export FAND_TEST=${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-drm-${CC}-${LIBC}
                            make test -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Build: CC=gcc DRM=n test'
                        sh '''
                            export CC=gcc
                            export FAND_TEST=${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-${CC}-${LIBC}
                            make test drm_support=n -j$(nproc) -B builddir=${BUILDDIR}
                        '''

                        echo 'Build: CC=clang DRM=n test'
                        sh '''
                            export CC=clang
                            export FAND_TEST=${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-${CC}-${LIBC}
                            make test drm_support=n -j$(nproc) -B builddir=${BUILDDIR}
                        '''
                    }
                }
            }
        }
        stage('Test') {
            failFast true
            parallel {
                stage('Test Musl') {
                    agent {
                        docker { image "${MUSL_DOCKER_IMAGE}" }
                    }
                    environment {
                        LIBC='musl'
                    }
                    steps {
                        echo '-- Starting musl tests --'

                        echo 'Test: CC=gcc DRM=y'
                        sh '${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-drm-gcc-${LIBC}'

                        echo 'Test: CC=clang DRM=y'
                        sh '${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-drm-clang-${LIBC}'

                        echo 'Test: CC=gcc DRM=n'
                        sh '${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-gcc-${LIBC}'

                        echo 'Test: CC=clang DRM=n'
                        sh '${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-clang-${LIBC}'
                    }
                }
                stage('Test Glibc') {
                    agent {
                        docker { image "${GLIBC_DOCKER_IMAGE}" }
                    }
                    environment {
                        LIBC='glibc'
                    }
                    steps {
                        echo '-- Starting glibc tests --'

                        echo 'Test: CC=gcc DRM=y'
                        sh '${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-drm-gcc-${LIBC}'

                        echo 'Test: CC=clang DRM=y'
                        sh '${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-drm-clang-${LIBC}'

                        echo 'Test: CC=gcc DRM=n'
                        sh '${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-gcc-${LIBC}'

                        echo 'Test: CC=clang DRM=n'
                        sh '${TEST_DIR}/${TEST_STEM}.${BUILD_NUMBER}-clang-${LIBC}'
                    }
                }
            }
        }
        stage('Build Fuzzer') {
            agent {
                docker { image "${GLIBC_DOCKER_IMAGE}" }
            }
            environment {
                LIBC='glibc'
            }
            steps {
                echo 'Creating fuzz directory'
                sh 'mkdir -p ${FUZZ_DIR}'

                echo 'Build: CC=clang fuzz server'
                sh '''
                    export FUZZIFACE=server
                    export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-${FUZZIFACE}-clang-${LIBC}
                    make fuzz -j$(nproc) -B
                '''

                echo 'Build: CC=clang fuzz cache'
                sh '''
                    export FUZZIFACE=cache
                    export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-${FUZZIFACE}-clang-${LIBC}
                    make fuzz -j$(nproc) -B
                '''

                echo 'Build: CC=clang fuzz config'
                sh '''
                    export FUZZIFACE=config
                    export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-${FUZZIFACE}-clang-${LIBC}
                    make fuzz -j$(nproc) -B
                '''

                echo 'Build: CC=clang fuzz client'
                sh '''
                    export FUZZIFACE=client
                    export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-${FUZZIFACE}-clang-${LIBC}
                    make fuzz -j$(nproc) -B
                '''
            }
        }
        stage('Fetch and Merge Corpora') {
            agent {
                docker { image "${GLIBC_DOCKER_IMAGE}" }
            }
            environment {
                LIBC='glibc'
            }
            steps {
                echo 'Copying existing corpora'
                sh 'mkdir -p ${ARTIFACT_DIR}'
                copyArtifacts filter: "${ARTIFACT_DIR}/corpora.zip", projectName: "${JOB_NAME}", fingerprintArtifacts: true, optional: true
                script {
                    if(fileExists("${ARTIFACT_DIR}/corpora.zip")) {
                        unzip zipFile: "${ARTIFACT_DIR}/corpora.zip", dir: "${FUZZ_DIR}"

                        echo 'Merging server corpora'
                        sh '''
                            export FUZZIFACE=server
                            export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-${FUZZIFACE}-clang-${LIBC}
                            make fuzzmerge CORPUS_ARTIFACTS=${FUZZ_DIR}/${FUZZIFACE}_corpora
                        '''

                        echo 'Merging cache corpora'
                        sh '''
                            export FUZZIFACE=cache
                            export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-${FUZZIFACE}-clang-${LIBC}
                            make fuzzmerge CORPUS_ARTIFACTS=${FUZZ_DIR}/${FUZZIFACE}_corpora
                        '''

                        echo 'Merging config corpora'
                        sh '''
                            export FUZZIFACE=config
                            export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-${FUZZIFACE}-clang-${LIBC}
                            make fuzzmerge CORPUS_ARTIFACTS=${FUZZ_DIR}/${FUZZIFACE}_corpora
                        '''

                        echo 'Merging client corpora'
                        sh '''
                            export FUZZIFACE=client
                            export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-${FUZZIFACE}-clang-${LIBC}
                            make fuzzmerge CORPUS_ARTIFACTS=${FUZZ_DIR}/${FUZZIFACE}_corpora
                        '''
                    }
                    else {
                        echo 'No corpora found'
                    }
                }
            }
        }
        stage('Run Fuzzer') {
            agent {
                docker { image "${GLIBC_DOCKER_IMAGE}" }
            }
            environment {
                LIBC='glibc'
            }
            steps {
                echo '-- Starting fuzzing of server interface --'
                sh '''
                    export FUZZIFACE=server
                    export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-${FUZZIFACE}-clang-${LIBC}
                    make fuzzrun
                '''
                echo '-- Starting fuzzing of cache interface --'
                sh '''
                    export FUZZIFACE=cache
                    export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-${FUZZIFACE}-clang-${LIBC}
                    make fuzzrun
                '''

                echo '-- Starting fuzzing of config interface --'
                sh '''
                    export FUZZIFACE=config
                    export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-${FUZZIFACE}-clang-${LIBC}
                    make fuzzrun
                '''

                echo '-- Starting fuzzing of client interface --'
                sh '''
                    export FUZZIFACE=client
                    export FAND_FUZZ=${FUZZ_DIR}/${FUZZ_STEM}.${BUILD_NUMBER}-${FUZZIFACE}-clang-${LIBC}
                    make fuzzrun
                '''
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
            node(null) {
                sh '''
                    mkdir -p ${ARTIFACT_DIR}/fuzz
                    cp -r src/fuzz/server/corpora ${ARTIFACT_DIR}/fuzz/server_corpora
                    cp -r src/fuzz/cache/corpora ${ARTIFACT_DIR}/fuzz/cache_corpora
                    cp -r src/fuzz/config/corpora ${ARTIFACT_DIR}/fuzz/config_corpora
                    cp -r src/fuzz/client/corpora ${ARTIFACT_DIR}/fuzz/client_corpora
                '''
                zip zipFile: "${ARTIFACT_DIR}/corpora.zip", archive: true, dir: "${ARTIFACT_DIR}/fuzz", overwrite: true
                archiveArtifacts artifacts: "${ARTIFACT_DIR}/corpora.zip", fingerprint: true

                echo 'Removing dangling Docker images'
                sh 'docker system prune -f'

                echo 'Cleaning up'
                deleteDir()
            }
        }
    }
}

