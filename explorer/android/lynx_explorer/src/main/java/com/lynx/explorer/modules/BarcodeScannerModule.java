package com.lynx.explorer.modules;

import android.content.Context;
import android.content.Intent;

import com.lynx.jsbridge.LynxMethod;
import com.lynx.jsbridge.LynxModule;
import com.lynx.explorer.scan.QRScanActivity;

public class BarcodeScannerModule extends LynxModule {
    public BarcodeScannerModule(Context context) {
        super(context);
    }

    @LynxMethod
    public void startScan() {
        Context ctx = mContext.getContext();
        Intent intent = new Intent(ctx, QRScanActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        ctx.startActivity(intent);
    }
}
