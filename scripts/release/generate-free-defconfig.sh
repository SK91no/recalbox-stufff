#!/bin/bash

set -uo pipefail

defconfig="configs/recalbox-$1_defconfig"
lite_defconfig="configs/recalbox-$1_lite_defconfig"

cp "${defconfig}" "${lite_defconfig}"

exitCode=0
for packageRaw in $(cat "${defconfig}" | grep -e "^BR2_PACKAGE" | sed 's/=.*//g;');do
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
		echo "INFO: non commercial license found in ${file}, removing package from defconfig"
    if grep -q "${package}" "${lite_defconfig}"; then
      echo "INFO: replacing ${package} in lite_defconfig"
      sed -i "s/${package}.*/${package}=n/g" "${lite_defconfig}"
    else
      echo "INFO: add ${package}=n to lite_defconfig"
      echo "${package}=n" >> "${lite_defconfig}"
    fi
	fi
	echo "INFO: name: $package $nc"
done

exit "${exitCode}"