pipeline {
    agent {
        docker { image 'build/minimal' }
    }
    environment {
        CC='gcc'
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

                echo 'Build: CC=gcc'
                sh 'make FAND=${ARTIFACT_DIR}/amdgpu-fand-3.0-gcc FANCTL=${ARTIFACT_DIR}/amdgpu-fanctl-3.0-gcc -j$(nproc) -B'

                echo 'Build: CC=clang'
                sh 'make CC=clang FAND=${ARTIFACT_DIR}/amdgpu-fand-3.0-clang FANCTL=${ARTIFACT_DIR}/amdgpu-fanctl-3.0-clang -j$(nproc) -B'
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

