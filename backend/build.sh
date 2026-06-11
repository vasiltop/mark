#!/bin/sh

set -e

root="$(cd "$(dirname "$0")" && pwd)"
/tmp/apache-maven-3.9.6/bin/mvn -f "$root/pom.xml" -q package -DskipTests
