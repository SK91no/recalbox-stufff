#!/bin/bash

set -uo pipefail

exitCode=0
for packageRaw in $(cat output/.config | grep -e "^BR2_PACKAGE" | sed 's/=.*//g;');do
	package="${packageRaw//BR2_PACKAGE_/}" 
	packageLower="${package,,}"
	packageDir="${packageLower//_/-}" 
	packageCustomMk="./package/${packageDir}/${packageDir}.mk"
	packageCustomMkUnderscore="./package/${packageDir}/${packageLower}.mk"

    file=""
	if [ -f "${packageCustomMk}" ];then	
		file="${packageCustomMk}"
	elif [ -f "${packageCustomMkUnderscore}" ];then	
		file="${packageCustomMkUnderscore}"
	else
		continue
	fi

	nc=""
	ncLine=$(grep -e "^${package}_NON_COMMERCIAL = y" "${file}")
	if [[ $? == 0 ]]; then
		echo "WARN: non commercial license found in ${file}"
		exitCode=1
		nc="NON COMMERICAL"
	fi
	echo "INFO: name: $package $nc"
done

exit "${exitCode}"