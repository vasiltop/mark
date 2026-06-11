#!/bin/sh

set -e

root="$(cd "$(dirname "$0")" && pwd)"
mvn="${MAVEN:-mvn}"
command -v "$mvn" >/dev/null 2>&1 || mvn="/tmp/apache-maven-3.9.6/bin/mvn"
"$mvn" -f "$root/pom.xml" -q package -DskipTests
