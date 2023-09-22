/**
 * \file    Jenkinsfile
 * \brief   Jenkinsfile Groovy script
 *
 * Copyright (C) 2022 Deniz Eren <deniz.eren@outlook.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

node('jenkins_agent') {

    def sshport = "9922";
    def buildpath = "/var/tmp/$NODE_NAME";
    def projectpath = "/opt/project_repo";

    try {
        stage('Checkout and setup dev environment') {
            sh(script: """
                rm -rf $buildpath
                mkdir -p $buildpath
            """)

            checkout([$class: 'GitSCM',
                branches: [[name: '*/main']],
                extensions: [[$class: 'RelativeTargetDirectory',
                    relativeTargetDir: "$buildpath/dev-can-linux"]],
                userRemoteConfigs: [[
                    credentialsId: '',
                    url: '/opt/project_repo'
                ]]])

            def publisher = LastChanges.getLastChangesPublisher \
                null, "SIDE", "LINE", true, true, \
                "", "", "", "$buildpath/dev-can-linux", ""

            publisher.publishLastChanges()

            sh(script: """
                $projectpath/dev-qnx/ci/scripts/setup-dev-environment.sh \
                    -i /home/jenkins/Images \
                    -r /opt/project_repo/dev-qnx

                docker cp /opt/project_repo dev_env:/root/
            """)
        }

        stage('Testing with coverage') {
            sh(script: """
                $projectpath/dev-qnx/ci/scripts/start-emulation.sh \
                    -i /home/jenkins/Images \
                    -p $sshport

                $projectpath/dev-qnx/ci/scripts/build-exec-program.sh \
                    -v \
                    -b /data/home/root/build_coverage \
                    -c dev-can-linux \
                    -e /root/project_repo/tests/driver/common/env/dual-channel.env \
                    -r /root/project_repo \
                    -t Coverage \
                    -p $sshport

                docker exec --user root --workdir /root dev_env bash -c \
                    "source .profile \
                    && /root/dev-qnx/dev/scripts/setup-profile.sh \
                    && cd /data/home/root/build_coverage \
                    && ctest --output-junit test_results.xml || (exit 0)"

                $projectpath/dev-qnx/ci/scripts/stop-program.sh \
                    -b /data/home/root/build_coverage \
                    -k dev-can-linux \
                    -p $sshport

                $projectpath/dev-qnx/ci/scripts/stop-emulation.sh

                docker exec --user root --workdir /root dev_env bash -c \
                    "source .profile \
                    && /root/dev-qnx/dev/scripts/setup-profile.sh \
                    && cd /data/home/root/build_coverage \
                    && lcov --gcov-tool=\\\$QNX_HOST/usr/bin/ntox86_64-gcov \
                        -t \"test_coverage_results\" -o tests.info \
                        -c -d /data/home/root/build_coverage \
                        --base-directory=/root/project_repo \
                        --no-external --quiet \
                    && genhtml -o /data/home/root/build_coverage/cov-html \
                        tests.info --quiet"

                rm -rf $WORKSPACE/test-coverage
                rm -rf $WORKSPACE/test_results.xml

                docker cp \
                    dev_env:/data/home/root/build_coverage/cov-html \
                        $WORKSPACE/test-coverage

                docker cp \
                    dev_env:/data/home/root/build_coverage/test_results.xml \
                        $WORKSPACE/test_results.xml
            """)

            junit('test_results.xml')

            publishHTML(target : [allowMissing: false,
                alwaysLinkToLastBuild: true,
                keepAll: true,
                reportDir: 'test-coverage',
                reportFiles: 'index.html',
                reportName: 'Test Coverage Report',
                reportTitles: 'Test Coverage Report'])
        }

        stage('Testing with Valgrind') {
            sh(script: """
                rm -rf $WORKSPACE/valgrind-*.xml

                $projectpath/dev-qnx/ci/scripts/start-emulation.sh \
                    -i /home/jenkins/Images \
                    -p $sshport

                #
                # Run Valgrind with tool memcheck
                #
                rm -rf /data/home/root/build_memcheck

                export QNX_PREFIX_CMD="valgrind \
                    --tool=memcheck \
                    --leak-check=full \
                    --show-leak-kinds=all \
                    --track-origins=yes \
                    --verbose \
                    --xml=yes --xml-file=valgrind-memcheck.xml"

                $projectpath/dev-qnx/ci/scripts/build-exec-program.sh \
                    -v \
                    -b /data/home/root/build_memcheck \
                    -c dev-can-linux \
                    -e /root/project_repo/tests/driver/common/env/dual-channel.env \
                    -r /root/project_repo \
                    -t Profiling \
                    -s 1 \
                    -p $sshport

                docker exec --user root --workdir /root dev_env bash -c \
                    "source .profile \
                    && /root/dev-qnx/dev/scripts/setup-profile.sh \
                    && cd /data/home/root/build_memcheck \
                    && ctest --verbose || (exit 0)"

                $projectpath/dev-qnx/ci/scripts/stop-program.sh \
                    -b /data/home/root/build_memcheck \
                    -k memcheck-amd64-nto \
                    -p $sshport

                docker cp \
                    dev_env:/data/home/root/build_memcheck/valgrind-memcheck.xml \
                    $WORKSPACE/

                #
                # Run Valgrind with tool helgrind
                #
                rm -rf /data/home/root/build_helgrind

                export QNX_PREFIX_CMD="valgrind \
                    --tool=helgrind \
                    --history-level=full \
                    --conflict-cache-size=5000000 \
                    --verbose \
                    --xml=yes --xml-file=valgrind-helgrind.xml"

                $projectpath/dev-qnx/ci/scripts/build-exec-program.sh \
                    -v \
                    -b /data/home/root/build_helgrind \
                    -c dev-can-linux \
                    -e /root/project_repo/tests/driver/common/env/dual-channel.env \
                    -r /root/project_repo \
                    -t Profiling \
                    -s 1 \
                    -p $sshport

                docker exec --user root --workdir /root dev_env bash -c \
                    "source .profile \
                    && /root/dev-qnx/dev/scripts/setup-profile.sh \
                    && cd /data/home/root/build_helgrind \
                    && ctest --verbose || (exit 0)"

                $projectpath/dev-qnx/ci/scripts/stop-program.sh \
                    -b /data/home/root/build_helgrind \
                    -k helgrind-amd64-nto \
                    -p $sshport

                docker cp \
                    dev_env:/data/home/root/build_helgrind/valgrind-helgrind.xml \
                    $WORKSPACE/

                #
                # Run Valgrind with tool drd
                #
                rm -rf /data/home/root/build_drd

                export QNX_PREFIX_CMD="valgrind \
                    --tool=drd \
                    --verbose \
                    --xml=yes --xml-file=valgrind-drd.xml"

                $projectpath/dev-qnx/ci/scripts/build-exec-program.sh \
                    -v \
                    -b /data/home/root/build_drd \
                    -c dev-can-linux \
                    -e /root/project_repo/tests/driver/common/env/dual-channel.env \
                    -r /root/project_repo \
                    -t Profiling \
                    -s 1 \
                    -p $sshport

                docker exec --user root --workdir /root dev_env bash -c \
                    "source .profile \
                    && /root/dev-qnx/dev/scripts/setup-profile.sh \
                    && cd /data/home/root/build_drd \
                    && ctest --verbose || (exit 0)"

                $projectpath/dev-qnx/ci/scripts/stop-program.sh \
                    -b /data/home/root/build_drd \
                    -k drd-amd64-nto \
                    -p $sshport

                docker cp \
                    dev_env:/data/home/root/build_drd/valgrind-drd.xml \
                    $WORKSPACE/

                #
                # Run Valgrind with tool exp-sgcheck
                #
                rm -rf /data/home/root/build_exp-sgcheck

                export QNX_PREFIX_CMD="valgrind \
                    --tool=exp-sgcheck \
                    --verbose \
                    --xml=yes --xml-file=valgrind-exp-sgcheck.xml"

                $projectpath/dev-qnx/ci/scripts/build-exec-program.sh \
                    -v \
                    -b /data/home/root/build_exp-sgcheck \
                    -c dev-can-linux \
                    -e /root/project_repo/tests/driver/common/env/dual-channel.env \
                    -r /root/project_repo \
                    -t Profiling \
                    -s 1 \
                    -p $sshport

                docker exec --user root --workdir /root dev_env bash -c \
                    "source .profile \
                    && /root/dev-qnx/dev/scripts/setup-profile.sh \
                    && cd /data/home/root/build_exp-sgcheck \
                    && ctest --verbose || (exit 0)"

                $projectpath/dev-qnx/ci/scripts/stop-program.sh \
                    -b /data/home/root/build_exp-sgcheck \
                    -k exp-sgcheck-amd64-nto \
                    -p $sshport

                docker cp \
                    dev_env:/data/home/root/build_exp-sgcheck/valgrind-exp-sgcheck.xml \
                    $WORKSPACE/

                $projectpath/dev-qnx/ci/scripts/stop-emulation.sh

                # Have a copy of the repository in the workspace so Valgrind
                # source-code displays works
                rm -rf $WORKSPACE/dev-can-linux
                cp -r /opt/project_repo $WORKSPACE/dev-can-linux
            """)

            publishValgrind(pattern: 'valgrind-*.xml',
                sourceSubstitutionPaths: '/root/project_repo:dev-can-linux')
        }

        stage('Release') {
            sh(script: """
                docker exec --user root --workdir /root dev_env \
                    bash -c "source .profile \
                        && /root/dev-qnx/dev/scripts/setup-profile.sh \
                        && mkdir -p build_release \
                        && cd build_release \
                        && cmake -DSSH_PORT=$sshport -DCMAKE_BUILD_TYPE=Release \
                                -DBUILD_TESTING=OFF ../project_repo \
                        && make -j8 \
                        && cpack \
                        && DIR=\\\"Release/dev-can-linux/\\\$(date \\\"+%Y-%m-%d-%H%M%S%Z\\\")\\\" \
                        && mkdir -p \\\"\\\$DIR\\\" \
                        && cp dev-can-linux-*.tar.gz \\\"\\\$DIR\\\""

                docker cp \
                    dev_env:/root/build_release/Release/dev-can-linux \
                    /var/Release/
            """)
        }
    }
    catch (err) {
        currentBuild.result = 'FAILURE'
    }
    finally {
        stage('Clean-up') {
            sh(script: """
                docker stop dev_env qemu_env || (exit 0)
                docker rm dev_env qemu_env || (exit 0)

                rm -rf $buildpath
            """)
        }
    }
}
