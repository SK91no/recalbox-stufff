#!/bin/bash
SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`
source "${SCRIPTPATH}/moonlight.inc.sh"
source "${SCRIPTPATH}/generic.inc.sh"

moonlight_ip=$2

moonlight_config_dir=/recalbox/share/system/configs/moonlight
moonlight_gamesnames=$moonlightConf/gamelist.txt
moonlight_romsdir=/recalbox/share/roms/moonlight
moonlight_mapping="$moonlight_config_dir/mapping.conf"
moonlight_keydir="$moonlight_config_dir/keydir-$moonlight_ip"
mlConf="$moonlightConf/moonlight-$moonlight_ip.conf"
SEPERATOR=";"

listGames() {
  $SCRIPT list $moonlight_ip 2>/dev/null | grep '^[0-9][0-9.]' | cut -d '.' -f 1- | sed 's/^[0-9]*\. //'
}

check_adress() {
  if [ -z "${moonlight_ip}" ];
  then
    echo "You need use \"$SCRIPT $1 <host>\""
    echo "<host> can be empty (not recommended if you have several GFE hosts), an IP or a PC name"
    exit 1
  fi
}

mlFindGfeHosts () {
  tmpOut=/tmp/moonlightHosts
  echo "Listing available GFE servers :"
  getNvServers > $tmpOut
  while read line
  do
    info=`getNvServerInfo $(echo $line | cut -d ';' -f 2)`
    parseServerInfo $info
    rm $info
  done < $tmpOut
  
  nbHosts=$(cat $tmpOut | wc -l)
  if [ "$nbHosts" -gt "0" ]; then
    echo "You can now run $0 pair <host>"
    echo "<host> can be empty (not recommended if you have several GFE hosts), an IP or a PC name"
  else
    echo "mlFindGfeHosts() : No GFE host was found" >&2
  fi
  rm $tmpOut
}


mlClean() {
  # remove all existing data concerning a GFE host
  host="$1"
  fullmode="$2"
  
  if [ -z $host ]; then
    conf="$moonlightConf/moonlight.conf"
    keydir="$moonlightConf/keydir"
  else
    conf="$moonlightConf/moonlight-$host.conf"
    keydir="$moonlightConf/keydir-$host"
  fi
  
  # Remove keydir, moonlight.conf in fullmode only. Sometimes, we just to remove games and scraping data, not the conf and keydir
  if [ "$fullmode" == "fullmode" ]; then
    rm $conf 2>/dev/null
    rm -rf $keydir 2>/dev/null
  fi
  
  ls ${mlRomDir}/*_$host.moonlight 2>/dev/null | while read rom; do
    shortRom=$(basename "$rom")
    # Remove rom
    rm "$rom"  
    
    # remove gamelist.txt entries
    grep -v "${moonlightSeparator}$shortRom$" $mlGameList > $mlGameList
    
    # remove scraping data
    hostInGamelist="$shortRom"
    # echo $hostInGamelist
    xml ed --inplace -d "//game[path[contains(text(),'$hostInGamelist')]]" $mlRomDir/gamelist.xml

  done
}


mlPair() {
  # Check $1 : IP or parameter or empty
  # and fill up internal variables
  isIp "$1" > /dev/null
  # IP mode : means the user may want to play through WAN, so don't look for any hostname
  echo "IP mode"
  mlIp="$moonlight_ip"
  mlConf="$moonlightConf/moonlight-$mlIp.conf"
  mlKeydir="$moonlightConf/keydir-$mlIp"
        
  echo "$mlHost($mlIp) $mlConf | $mlKeydir"
  
  # remove existing keydir for the host
  rm -rf $mlKeydir
  
  # We're all set, time to pair ! Exit if failed
  moonlight pair -keydir $mlKeydir $mlIp 
  [ $? != 0 ] && { echo "ERROR mlPair() : could not pair. Exiting ... " ; exit 1 ; }
  
  # Output some cool stuff
  info=$(getNvServerInfo $mlIp)
  parseServerInfo $info
  rm $info
  # Create the moonlight conf
  rm $mlConf 2>/dev/null
  cp /recalbox/share_init/system/configs/moonlight/moonlight.conf $mlConf
  sed -i "s+.*address =+address = $mlIp+" $mlConf 
  sed -i "s+.*keydir =.*+keydir = $mlKeydir+" $mlConf 
}

createRomLinks() {
  rm $moonlight_gamesnames
  rm -rf $moonlight_romsdir/$moonlight_ip/* 2>/dev/null

  listGames | while read line
  do
    filename=$(echo $line | sed 's/[^ A-Za-z0-9._-]/-/g')
    echo "$filename$SEPERATOR$line$SEPERATOR$mlConf" >> $moonlight_gamesnames
    mkdir -p "$moonlight_romsdir/$moonlight_ip"
    touch "$moonlight_romsdir/$moonlight_ip/$moonlight_ip_${filename}.moonlight"
  done
}

findRealGameName() {
  grep "$*" $moonlight_gamesnames | cut -d "$SEPERATOR" -f 2
}

scrape() {
  GDBURL="http://thegamesdb.net/api/GetGame.php?platform=pc&exactname="
  GAMELIST=$moonlight_romsdir/gamelist.xml
  IMGPATH=$moonlight_romsdir/$1/downloaded_images

  # Test if $GDBURL is online, and stop if it's offline
  dbdns=$(echo $GDBURL | awk -F/ '{print $3}')
  ping -c 1 $dbdns > /dev/null 2>&1
  if [ $? -ne '0' ]
  then
    echo "$dbdns is not online. Can't scrape" >&2
    exit
  fi

  # Make sure the $IMGPATH exists
  [ ! -d $IMGPATH ] && mkdir -p $IMGPATH


  # This is what we were waiting for : generate the gamelist.xml
  echo '<?xml version="1.0"?>' > $GAMELIST
  echo '<gameList>' >> $GAMELIST

  while read line
  do
    echo "Scraping games $gamename ..."
    # Get the real game name, not the moonlight link + prepare xml game data
    moonlightfilename=$(echo $line | cut -d ';' -f 1)
    xmlfilename=/tmp/${moonlightfilename}.xml
    gamename=$(echo $line | cut -d ';' -f 2)

    # download XML game data from TheGamesDB.net
    wget "$GDBURL$gamename" -O "$xmlfilename" >/dev/null 2>&1

    # Time to get values for the gamelist.xml
    id=$(xml sel -t -v "Data/Game/id" "$xmlfilename" 2>/dev/null)
    source="theGamesDB.net"
    path="./$moonlight_ip/$moonlight_ip_${moonlightfilename}.moonlight"
    desc=$(xml sel -t -v "Data/Game/Overview" "$xmlfilename" 2>/dev/null)

    # A few steps to get the cover art url
    imgurl=$(xml sel -t -v "Data/baseImgUrl" -v "Data/Game/Images/boxart[@side='front']/@thumb" "$xmlfilename" 2>/dev/null)
    extension="png"
    img=$IMGPATH/${gamename}.${extension}
    wget $imgurl -O "$img" >/dev/null 2>&1

    rating=$(xml sel -t -v "Data/Game/Rating" "$xmlfilename" 2>/dev/null)
    releasedate=$(xml sel -t -v "Data/Game/ReleaseDate" "$xmlfilename" 2>/dev/null | sed 's/^\([0-9]\{2\}\)\/\([0-9]\{2\}\)\/\([0-9]\{4\}\)/\3\1\2T0000/')
    developer=$(xml sel -t -v "Data/Game/Developer" "$xmlfilename" 2>/dev/null)
    publisher=$(xml sel -t -v "Data/Game/Publisher" "$xmlfilename" 2>/dev/null)
    genre=$(xml sel -T -t -m "Data/Game/Genres/genre" -v 'text()' -i 'not(position()=last())' -o ' / ' "$xmlfilename" 2>/dev/null)
    players=$(xml sel -t -v "Data/Game/Players" "$xmlfilename" 2>/dev/null)


    # Write the XML data
    cat << EOF >> $GAMELIST
  <game id="$id" source="$source">
    <path>$path</path>
    <name>$gamename ($1)</name>
    <desc>$desc</desc>
    <image>./downloaded_images/$1/${gamename}.${extension}</image>
    <rating>$rating</rating>
    <releasedate>$releasedate</releasedate>
    <developer>$developer</developer>
    <publisher>$publisher</publisher>
    <genre>$genre</genre>
    <players>$players</players>
  </game>

EOF
    rm "$xmlfilename"
  done < <(cat $moonlight_gamesnames | sed "s/\t/;/g")
  echo '</gameList>' >> $GAMELIST
}

mkdir -p $moonlight_keydir;

#
# Main section
#

case $1 in
  find)
    mlFindGfeHosts
    ;;
  init)
    check_adress $1
    echo "Fetching games from $moonlight_ip ..."
    createRomLinks
    scrape $moonlight_ip
    ;;
  pair)
    check_adress $1
    mlPair "$1"
    ;;
  list)
    check_adress $1
    cmd="moonlight list -keydir ${moonlightConf}/keydir-${moonlight_ip}" ;;
  clean)
    check_adress $1
    mlClean "$moonlight_ip" "fullmode"
    echo 'You can now pair again your recalbox with a PC'
    ;;
  *)
    echo "Unknown option $1" >&2
esac

$cmd
