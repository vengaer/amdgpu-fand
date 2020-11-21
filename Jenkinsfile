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
                sh 'make -j$(nproc)'

                echo 'Build: CC=clang'
                sh 'make -j$(nproc) CC=clang'
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

