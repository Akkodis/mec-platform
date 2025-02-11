#!/bin/bash
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

if [ "$#" -ne 2]; then
  echo "Usage: $0 <NEW_VERSION> <USER>"
  echo "Exapmle: $0 v16.0.0 garciadeblas"
  echo "Exapmle: $0 v15.0.7 vegall"
  exit 1
fi

NEW_VERSION="$1"
USER="$2"
REPO_URL="ssh://$USER@osm.etsi.org:29418/osm/devops"
# If the $NEW_VERSION == v15.0.1, the $BRANCH_NAME will be v15.0
BRANCH_NAME=$(echo $NEW_VERSION | grep -oE 'v[0-9]+\.[0-9]+')

git clone $REPO_URL
cd devops

git checkout $BRANCH_NAME

sed -i -E "0,/^version: .*/s//version: $NEW_VERSION/" installers/osm/Chart.yaml
sed -i -E "0,/^appVersion: .*/s//appVersion: \"$NEW_VERSION\"/" installers/helm/osm/Chart.yaml

git add installers/helm/osm/Chart.yaml
git commit -m "Update chart version version to $NEW_VERSION"
git push origin $BRANCH_NAME

commit=$(git show --summary | grep commit | awk '{print $2}')
echo "The commit is $commit"