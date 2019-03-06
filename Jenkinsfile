pipeline {
  agent {
    docker {
      image 'fpoussin/jenkins:ubuntu-18.04-qt5'
    }

  }
  stages {
    stage('Prepare code') {
      steps {
        sh '''git submodule sync
git submodule update --init'''
      }
    }
    stage('Build') {
      steps {
        sh 'nice make -j $(nproc)'
      }
    }
  }
}