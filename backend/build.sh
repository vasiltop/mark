#!/bin/sh

set -e

/tmp/apache-maven-3.9.6/bin/mvn -f backend/pom.xml -q package -DskipTests
