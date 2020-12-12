pipeline {
    agent {
        docker { image 'build/minimal' }
    }
    environment {
        CC='gcc'
        CFLAGS='-Werror'
        ARTIFACT_DIR='artifacts'
    }
    stages {
        stage('Gitlab Pending') {
            steps {
                echo 'Notifying Gitlab'
                updateGitlabCommitStatus name: 'build', state: 'pending'
            }
        }
        stage('Build') {
            steps {
                echo '-- Starting build --'

                echo 'Creating artifact directory'
                sh 'mkdir -p ${ARTIFACT_DIR}'

                echo 'Build: CC=gcc DRM=y'
                sh 'make FAND=${ARTIFACT_DIR}/amdgpu-fand-4.0-gcc FANCTL=${ARTIFACT_DIR}/amdgpu-fanctl-4.0-gcc -j$(nproc) -B'

                echo 'Build: CC=clang DRM=y'
                sh 'make CC=clang FAND=${ARTIFACT_DIR}/amdgpu-fand-4.0-clang FANCTL=${ARTIFACT_DIR}/amdgpu-fanctl-4.0-clang -j$(nproc) -B'

                echo 'Build: CC=gcc DRM=n'
                sh 'make FAND=${ARTIFACT_DIR}/amdgpu-fand-4.0-gcc FANCTL=${ARTIFACT_DIR}/amdgpu-fanctl-4.0-gcc drm_support=n -j$(nproc) -B'

                echo 'Build: CC=clang DRM=n'
                sh 'make CC=clang FAND=${ARTIFACT_DIR}/amdgpu-fand-4.0-clang FANCTL=${ARTIFACT_DIR}/amdgpu-fanctl-4.0-clang drm_support=n -j$(nproc) -B'
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

