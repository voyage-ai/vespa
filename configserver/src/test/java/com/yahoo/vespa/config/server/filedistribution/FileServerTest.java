// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.config.server.filedistribution;

import com.yahoo.cloud.config.ConfigserverConfig;
import com.yahoo.config.FileReference;
import com.yahoo.io.IOUtils;
import com.yahoo.jrt.Supervisor;
import com.yahoo.jrt.Transport;
import com.yahoo.net.HostName;
import com.yahoo.vespa.filedistribution.FileDownloader;
import com.yahoo.vespa.filedistribution.FileReferenceCompressor;
import com.yahoo.vespa.filedistribution.FileReferenceData;
import com.yahoo.vespa.filedistribution.FileReferenceDownload;
import com.yahoo.vespa.flags.InMemoryFlagSource;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import java.io.File;
import java.io.IOException;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;

import static com.yahoo.vespa.filedistribution.FileReferenceData.CompressionType.lz4;
import static com.yahoo.vespa.filedistribution.FileReferenceData.CompressionType.none;
import static com.yahoo.vespa.filedistribution.FileReferenceData.CompressionType.zstd;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

public class FileServerTest {

    private FileServer fileServer;

    @Rule
    public TemporaryFolder temporaryFolder = new TemporaryFolder();

    @Before
    public void setup() throws IOException {
        File rootDir = new File(temporaryFolder.newFolder("fileserver-root").getAbsolutePath());
        fileServer = new FileServer(new MockFileDownloader(rootDir), List.of(lz4, zstd, none), new FileDirectory(rootDir));
    }

    @Test
    public void requireThatExistingFileIsFound() throws IOException {
        String dir = "123";
        writeFile(dir);
        assertTrue(fileServer.hasFile(dir));
    }

    @Test
    public void requireThatNonExistingFileIsNotFound() {
        assertFalse(fileServer.hasFile("12x"));
    }

    @Test
    public void requireThatNonExistingFileWillBeDownloaded() throws IOException {
        String dir = "123";
        assertFalse(fileServer.hasFile(dir));
        FileReferenceDownload foo = new FileReferenceDownload(new FileReference(dir), "test");
        assertFalse(fileServer.getFileDownloadIfNeeded(foo).isPresent());
        writeFile(dir);
        assertTrue(fileServer.getFileDownloadIfNeeded(foo).isPresent());
    }

    @Test
    public void requireThatFileReferenceWithDirectoryCanBeFound() throws IOException {
        File dir = getFileServerRootDir();
        IOUtils.writeFile(dir + "/124/subdir/f1", "test", false);
        IOUtils.writeFile(dir + "/124/subdir/f2", "test", false);
        assertTrue(fileServer.hasFile("124/subdir"));
    }

    @Test
    public void requireThatWeCanReplayFileWithZstd() throws IOException, InterruptedException, ExecutionException {
        requireThatWeCanReplayFile(zstd);
    }

    @Test
    public void requireThatWeCanReplayFileWithLz4() throws IOException, InterruptedException, ExecutionException {
        requireThatWeCanReplayFile(lz4);
    }

    @Test
    public void requireThatWeCanReplayFileWithNoCompression() throws IOException, InterruptedException, ExecutionException {
        requireThatWeCanReplayFile(none);
    }

    @Test
    public void requireThatWeCanReplayDirWithLz4() throws IOException, InterruptedException, ExecutionException {
        File rootDir = new File(temporaryFolder.newFolder("fileserver-root-3").getAbsolutePath());
        fileServer = new FileServer(new MockFileDownloader(rootDir), List.of(lz4, zstd), new FileDirectory(rootDir)); // prefer lz4
        File dir = getFileServerRootDir();
        IOUtils.writeFile(dir + "/subdir/12z/f1", "dummy-data-2", true);
        CompletableFuture<byte []> content = new CompletableFuture<>();
        FileReference fileReference = new FileReference("subdir");
        var file = fileServer.getFileDownloadIfNeeded(new FileReferenceDownload(fileReference, "test"));
        fileServer.startFileServing(fileReference, file.get(), new FileReceiver(content), Set.of(zstd, lz4));

        // Decompress with lz4 and check contents
        var compressor = new FileReferenceCompressor(FileReferenceData.Type.compressed, lz4);
        File downloadedFileCompressed = new File(dir + "/downloaded-file-compressed");
        IOUtils.writeFile(downloadedFileCompressed, content.get());
        File downloadedFileUncompressed = new File(dir + "/downloaded-file-uncompressed");
        compressor.decompress(downloadedFileCompressed, downloadedFileUncompressed);
        assertTrue(downloadedFileUncompressed.isDirectory());
        assertEquals("dummy-data-2", IOUtils.readFile(new File(downloadedFileUncompressed, "12z/f1")));
    }

    @Test
    public void requireThatDifferentNumberOfConfigServersWork() throws IOException {
        // Empty connection pool in tests etc.
        ConfigserverConfig.Builder builder = new ConfigserverConfig.Builder()
                .configServerDBDir(temporaryFolder.newFolder("serverdb").getAbsolutePath())
                .configDefinitionsDir(temporaryFolder.newFolder("configdefinitions").getAbsolutePath());
        FileServer fileServer = createFileServer(builder);
        assertEquals(0, fileServer.downloader().connectionPool().getSize());

        // Empty connection pool when only one server, no use in downloading from yourself
        List<ConfigserverConfig.Zookeeperserver.Builder> servers = new ArrayList<>();
        ConfigserverConfig.Zookeeperserver.Builder serverBuilder = new ConfigserverConfig.Zookeeperserver.Builder();
        serverBuilder.hostname(HostName.getLocalhost());
        serverBuilder.port(123456);
        servers.add(serverBuilder);
        builder.zookeeperserver(servers);
        fileServer = createFileServer(builder);
        assertEquals(0, fileServer.downloader().connectionPool().getSize());

        // connection pool of size 1 when 2 servers
        ConfigserverConfig.Zookeeperserver.Builder serverBuilder2 = new ConfigserverConfig.Zookeeperserver.Builder();
        serverBuilder2.hostname("bar");
        serverBuilder2.port(123456);
        servers.add(serverBuilder2);
        builder.zookeeperserver(servers);
        fileServer = createFileServer(builder);
        assertEquals(1, fileServer.downloader().connectionPool().getSize());
    }

    @Test
    public void requireThatErrorsAreHandled() throws IOException, ExecutionException, InterruptedException {
        File dir = getFileServerRootDir();
        IOUtils.writeFile(dir + "/12y/f1", "dummy-data", true);
        CompletableFuture<byte []> content = new CompletableFuture<>();
        FailingFileReceiver fileReceiver = new FailingFileReceiver(content);

        // Should fail the first time, see FailingFileReceiver
        FileReference reference = new FileReference("12y");
        var file = fileServer.getFileDownloadIfNeeded(new FileReferenceDownload(reference, "test"));
        try {
            fileServer.startFileServing(reference, file.get(), fileReceiver, Set.of(lz4));
            fail("Should have failed");
        } catch (RuntimeException e) {
            // expected
        }

        fileServer.startFileServing(reference, file.get(), fileReceiver, Set.of(lz4));
        assertEquals(new String(content.get()), "dummy-data");
    }

    private void writeFile(String dir) throws IOException {
        File rootDir = getFileServerRootDir();
        IOUtils.createDirectory(rootDir + "/" + dir);
        IOUtils.writeFile(rootDir + "/" + dir + "/f1", "test", false);
    }

    private FileServer createFileServer(ConfigserverConfig.Builder configBuilder) throws IOException {
        File fileReferencesDir = temporaryFolder.newFolder();
        configBuilder.fileReferencesDir(fileReferencesDir.getAbsolutePath());
        return new FileServer(new ConfigserverConfig(configBuilder), new InMemoryFlagSource(), new FileDirectory(fileReferencesDir));
    }

    private static class FileReceiver implements FileServer.Receiver {
        final CompletableFuture<byte []> content;
        FileReceiver(CompletableFuture<byte []> content) {
            this.content = content;
        }
        @Override
        public void receive(FileReferenceData fileData, FileServer.ReplayStatus status) {
            this.content.complete(fileData.content().array());
        }
    }

    private static class FailingFileReceiver implements FileServer.Receiver {
        final CompletableFuture<byte []> content;
        int counter = 0;
        FailingFileReceiver(CompletableFuture<byte []> content) {
            this.content = content;
        }
        @Override
        public void receive(FileReferenceData fileData, FileServer.ReplayStatus status) {
            counter++;
            if (counter <= 1)
                throw new RuntimeException("Failed to receive file");
            else {
                this.content.complete(fileData.content().array());
            }
        }
    }

    private File getFileServerRootDir() {
        return fileServer.getRootDir().getRoot();
    }


    private void requireThatWeCanReplayFile(FileReferenceData.CompressionType compressionType) throws IOException, InterruptedException, ExecutionException {
        File dir = getFileServerRootDir();
        IOUtils.writeFile(dir + "/12y/f1", "dummy-data", true);
        CompletableFuture<byte []> content = new CompletableFuture<>();
        FileReference fileReference = new FileReference("12y");
        var file = fileServer.getFileDownloadIfNeeded(new FileReferenceDownload(fileReference, "test"));
        fileServer.startFileServing(fileReference, file.get(), new FileReceiver(content), Set.of(compressionType));
        assertEquals(new String(content.get()), "dummy-data");
    }

    private static class MockFileDownloader extends FileDownloader {

        public MockFileDownloader(File downloadDirectory) {
            super(FileDownloader.emptyConnectionPool(),
                  new Supervisor(new Transport("mock")).setDropEmptyBuffers(true),
                  downloadDirectory,
                  Duration.ofMillis(1000),
                  Duration.ofMillis(100));
        }

    }

}
