package com.mark;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import org.springframework.stereotype.Component;

@Component
public class AssetStore {
  private final Map<String, StoredAsset> assets = new ConcurrentHashMap<>();

  public void save(String id, StoredAsset asset) {
    assets.put(id, asset);
  }

  public StoredAsset get(String id) {
    return assets.get(id);
  }

  public record StoredAsset(String filename, String mimeType, String dataBase64) {}
}
