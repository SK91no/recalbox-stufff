#!/bin/bash

while read -r _ENV; do
  export "${_ENV?}"
done < /etc/environment
