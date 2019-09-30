/* vim: set foldmethod=marker foldmarker={,} foldlevel=42 filetype=groovy ts=4 et sw=4 sts=4: */
pipeline {
    agent none
    stages {
        stage('trigger internal build') {
            steps {
                build job: 'FLUFFI multibranch/master', wait: false
            }
        }
    }
}
