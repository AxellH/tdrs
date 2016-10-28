#!/bin/bash

TDRS_BIN=/usr/local/bin/tdrs

for param in $($TDRS_BIN --help | egrep -o "(--[a-zA-Z0-9-]+)" | sort | uniq)
do
	ENV_PARAM_NAME=$(echo $param | sed 's/--/TDRS_/' | sed 's/-/_/g' | tr '[:lower:]' '[:upper:]')

	if [[ -n ${!ENV_PARAM_NAME} ]]
	then
		TDRS_ARGS_LINE="$TDRS_ARGS_LINE $param ${!ENV_PARAM_NAME}"
	fi
done

$TDRS_BIN $TDRS_ARGS_LINE
