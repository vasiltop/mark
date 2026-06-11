package com.mark;

import java.util.Base64;
import java.util.Map;
import java.util.UUID;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.multipart.MultipartFile;
import org.springframework.web.server.ResponseStatusException;

@RestController
@RequestMapping("/api/assets")
public class AssetController {
  private final AssetStore store;

  public AssetController(AssetStore store) {
    this.store = store;
  }

  @PostMapping
  @ResponseStatus(HttpStatus.CREATED)
  public Map<String, String> upload(@RequestParam("file") MultipartFile file) {
    if (file.isEmpty()) {
      throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "file is required");
    }
    try {
      var id = UUID.randomUUID().toString();
      var mime = file.getContentType() != null ? file.getContentType() : "application/octet-stream";
      var data = Base64.getEncoder().encodeToString(file.getBytes());
      store.save(id, new AssetStore.StoredAsset(file.getOriginalFilename(), mime, data));
      return Map.of(
          "assetId", id,
          "filename", file.getOriginalFilename() != null ? file.getOriginalFilename() : "asset"
      );
    } catch (Exception e) {
      throw new ResponseStatusException(HttpStatus.INTERNAL_SERVER_ERROR, "upload failed");
    }
  }
}
