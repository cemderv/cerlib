// TODO: Replace this by your company name and game name.
package com.MyCompany.MyGame;

import android.content.res.AssetManager;
import android.os.Bundle;
import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity {

  private AssetManager assetManager;

  @Override
  protected String[] getLibraries() {
    // TODO: Change "MyGame" to the name of your game.
    // The order of the array elements must remain.
    return new String[] { "SDL3", "cerlib", "MyGame" };
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    assetManager = getResources().getAssets();
    setAssetManager(assetManager);
  }

  private static native void setAssetManager(AssetManager assetManager);
}
