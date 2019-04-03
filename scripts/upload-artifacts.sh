#!/bin/bash
set -e

# export BUILD_FILE=/usr/src/input/build-${ARCH}/out/build/outputs/apk/release/out-release-signed.apk
export BUILD_FILE=build-${ARCH}/out//build/outputs/apk/debug/out-debug.apk
export GITHUB_REPO=lutraconsulting/input

# If we have secure env vars and are in either a pull request or a tag, we need to upload artifacts
if [[ "${TRAVIS_SECURE_ENV_VARS}" = "true" ]];
then
  if [ ${TRAVIS_PULL_REQUEST} != false ]; then
    export DROPBOX_FOLDER="pulls"
    export APK_FILE=input-${TRAVIS_PULL_REQUEST}-${TRAVIS_COMMIT}-${ARCH}.apk
    export GITHUB_API=https://api.github.com/repos/${GITHUB_REPO}/issues/${TRAVIS_PULL_REQUEST}/comments
  elif [[ -n ${TRAVIS_TAG} ]]; then
    export DROPBOX_FOLDER="tags"
    export APK_FILE=input-${TRAVIS_TAG}-${TRAVIS_COMMIT}-${ARCH}.apk
    export GITHUB_API=https://api.github.com/repos/${GITHUB_REPO}/commits/${TRAVIS_COMMIT}/comments
  elif [[ ${TRAVIS_BRANCH} = master ]]; then
    export DROPBOX_FOLDER="master"
    export APK_FILE=input-${TRAVIS_BRANCH}-${TRAVIS_COMMIT}-${ARCH}.apk
    export GITHUB_API=https://api.github.com/repos/${GITHUB_REPO}/commits/${TRAVIS_COMMIT}/comments
  fi

  sudo cp ${BUILD_FILE} /tmp/${APK_FILE}
  APK_URL=`python3 ./scripts/uploader.py --source /tmp/${APK_FILE} --destination "$DROPBOX_FOLDER/${APK_FILE}" --token DROPBOX_TOKEN | tail -n 1`
  curl -u inputapp-bot:${GITHUB_TOKEN} -X POST --data '{"body": "Apk ready for [armv7](${APK_URL})"}' https://api.github.com/repos/${GITHUB_REPO}/issues/${TRAVIS_PULL_REQUEST}/comments

else
  echo -e "Not uploading artifacts ..."
  if [ "${TRAVIS_SECURE_ENV_VARS}" != "true" ];
  then
    echo -e "  TRAVIS_SECURE_ENV_VARS is not true (${TRAVIS_SECURE_ENV_VARS})"
  fi
  if [ "${TRAVIS_SECURE_ENV_VARS}" = "false" ];
  then
    echo -e "  TRAVIS_PULL_REQUEST is false (${TRAVIS_PULL_REQUEST})"
  fi
  if [ "${TRAVIS_TAG}X" = "X" ];
  then
    echo -e "  TRAVIS_TAG is not set"
  fi
fi