#!/bin/sh
VERSION="1.41.1"

TMPDIR="$XDG_RUNTIME_DIR"
DIR="$TMPDIR/codium"

CODIUM="$DIR/bin/codium --extensions-dir=$DIR/extensions --user-data-dir=$DIR/data"

function installCodium() {
    mkdir -p ~/.config/VSCodium-portable/
    touch ~/.config/VSCodium-portable/{keybindins.json,settings.json}
    mkdir -p "$DIR"


    echo "In directory: "
    pushd "$TMPDIR/codium"
    echo "Downloading"
    curl -O -L https://istina.msu.ru/media/common/VSCodium-linux-x64-$VERSION.tar.gz
    echo "Unpacking"
    tar xf VSCodium-linux-x64-$VERSION.tar.gz

    mkdir -p extensions
    mkdir -p data/user-data/User/

    ln -s "$TMPDIR/codium" ~/.vscode/
    mkdir "$TMPDIR/codium/dot-config"
    ln -s "$TMPDIR/codium/dot-config" ~/.config/Code
    mkdir "$TMPDIR/codium/dot-cache"
    ln -s "$TMPDIR/codium/dot-cache" ~/.cache/vscode-cpptools    
    # Save preferences
    ln -s ~/.config/VSCodium-portable/keybindins.json data/user-data/User/
    ln -s ~/.config/VSCodium-portable/settings.json data/user-data/User/

    $CODIUM --force --install-extension eamodio.gitlens
    $CODIUM --force --install-extension ms-vscode.cpptools
    popd
}

if [ ! -f $DIR/codium ]; then
    installCodium
fi

$CODIUM "$@"
