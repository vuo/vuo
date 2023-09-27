<?php

use Behat\Behat\Context\Context;
use Behat\Gherkin\Node\PyStringNode;
use Behat\Gherkin\Node\TableNode;

class FeatureContext implements Context {
  private $testDir = '';
  private $arch = '';

  public function __construct($sourceDir = '../..', $testDir = '../../build/test/TestBuildSystem') {
    $this->testDir = $testDir;

    // Clone from the source tree.
    // (Don't `git clone`, to support testing uncommitted changes.)
    static $updatedSourceTree = false;
    if (!$updatedSourceTree) {
      print "  Updating source tree…\n";

      @mkdir($testDir);
      @mkdir($testDir . '/src');

      if (is_file($testDir . '/src/type/list/VuoList_VuoInteger.cc')) {
        exec('chmod -R +w ' . escapeshellarg($testDir . '/src/type/list'), $output, $returnCode);
        if ($returnCode != 0)
          throw new Exception("chmod failed");
      }

      # `--inplace` keeps rsync from breaking the hardlink between the source tree and `Vuo.framework/Headers/*`.
      exec('rsync -a --inplace --delete --exclude build ' . escapeshellarg($sourceDir . '/') . ' ' . escapeshellarg($testDir . '/src'), $output, $returnCode);
      if ($returnCode != 0)
        throw new Exception("rsync failed");

      @mkdir($testDir . '/src/build');
      $this->iRunCMake();
      print "\n";

      $updatedSourceTree = true;
    }

    $this->arch = trim(shell_exec('/usr/bin/uname -m'));
    if (!in_array($this->arch, ['x86_64', 'arm64']))
      throw new Exception("uname failed: {$this->arch}");
  }

  private $lastBuildStarted = INF;
  private $lastBuildOutput = [];

  private function filterOutput(&$output) {
    $output = array_filter($output, function($v) {
      return strpos($v, '-- Vuo build:') === false
          && strpos($v, '-- Building to run on:') === false
          && strpos($v, '-- Enabled support for Vuo Pro.') === false
          && strpos($v, 'Configuring done') === false
          && strpos($v, 'Generating done') === false
          && strpos($v, 'Build files have been written to:') === false
          && strpos($v, 'Built target ') === false
          && strpos($v, 'Scanning dependencies of target') === false
          && strpos($v, 'Consolidate compiler generated dependencies') === false
          && strpos($v, 'Automatic MOC ') === false;
    });
  }

  private function build($args) {
    $this->lastBuildStarted = time();
    exec('cd ' . escapeshellarg($this->testDir . '/src/build') . ' && CLICOLOR_FORCE=1 /usr/bin/make ' . $args . ' 2>&1', $output, $returnCode);
    $this->filterOutput($output);
    $this->lastBuildOutput = $output;
    foreach ($output as $l)
      print $l . "\n";
    if ($returnCode != 0)
      throw new Exception("build failed");
  }

  /**
   * @Given I clean the build
   */
  public function givenICleanTheBuild() {
    $this->build('clean');
  }

  /**
   * @Given a completed build
   */
  public function givenACompletedBuild() {
    $this->iBuild();
  }

  /**
   * @When /I edit (.*)$/
   *
   * Relative to the source tree.
   */
  public function iEditAFile($file) {
    $file = $this->testDir . '/src/' . $file;
    if (!is_file($file))
      throw new Exception("Can't find file \"$file\".");
    if (!touch($file))
      throw new Exception("touch \"$file\" failed.");
  }

  /**
   * @When /I run CMake with arguments "([^"]*)"/
   */
  public function iRunCMakeWithArguments($args) {
    exec('cd ' . escapeshellarg($this->testDir . '/src/build') . ' && cmake -D CMAKE_C_COMPILER_LAUNCHER=ccache -D CMAKE_CXX_COMPILER_LAUNCHER=ccache .. ' . $args . ' 2>&1', $output, $returnCode);
    $this->filterOutput($output);
    foreach ($output as $l)
      print $l . "\n";
    if ($returnCode != 0)
      throw new Exception("CMake failed");
  }

  /**
   * @When I run CMake
   */
  public function iRunCMake() {
    $this->iRunCMakeWithArguments('');
  }

  /**
   * @When /I build with arguments "([^"]*)"/
   */
  public function iBuildWithArguments($args) {
    $this->build($args);
  }

  /**
   * @When I build
   */
  public function iBuild() {
    $this->build('-j' . trim(shell_exec('sysctl -n hw.ncpu')));
  }

  /**
   * @Then it shouldn't do anything
   */
  public function itShouldntDoAnything() {
    if (count($this->lastBuildOutput))
      throw new Exception("The most recent build shouldn't have changed anything, but it did.");
  }

  /**
   * @Then /it should update ([^ ]*)$/
   *
   * Relative to the build folder.
   */
  public function itShouldUpdateAFile($file) {
    if (time() < $this->lastBuildStarted)
      throw new Exception("'When I build' hasn't been called yet.");
    $buildFile = $this->testDir . '/src/build/' . $file;
    if (!is_file($buildFile))
      throw new Exception("Can't find build file \"$file\".");
	$mtime = stat($buildFile)['mtime'];
	if ($mtime < $this->lastBuildStarted)
	  throw new Exception("I expected the build to update \"$file\", but it didn't.  buildStarted={$this->lastBuildStarted}, mtime=$mtime");
  }

  /**
   * @Then /it shouldn't update ([^ ]*)$/
   *
   * Relative to the build folder.
   */
  public function itShouldntUpdateAFile($file) {
    if (time() < $this->lastBuildStarted)
      throw new Exception("'When I build' hasn't been called yet.");
    $buildFile = $this->testDir . '/src/build/' . $file;
    if (!is_file($buildFile))
      throw new Exception("Can't find build file \"$file\".");
    if (stat($buildFile)['mtime'] > $this->lastBuildStarted)
      throw new Exception("I expected the build to not update \"$file\", but it did.");
  }

  /**
   * @Then /it should update the ([^ ]*) node set$/
   */
  public function itShouldUpdateANodeSet($nodeSet) {
    $this->itShouldUpdateAFile("node/$nodeSet/$nodeSet.vuonode");
    $this->itShouldUpdateAFile("lib/Vuo.framework/Modules/$nodeSet.vuonode");
    $this->itShouldUpdateAFile("bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/$nodeSet.vuonode");
  }

  /**
   * @Then it should update the built-in cache
   */
  public function itShouldUpdateTheBuiltInCache() {
    $this->itShouldUpdateAFile('lib/Vuo.framework/Modules/Builtin/libVuoModuleCache.dylib');
    $this->itShouldUpdateAFile('lib/Vuo.framework/Modules/Builtin/manifest.txt');
    $this->itShouldUpdateAFile('bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/Builtin/libVuoModuleCache.dylib');
    $this->itShouldUpdateAFile('bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/Builtin/manifest.txt');
  }

  /**
   * @Then it shouldn't update the built-in cache
   */
  public function itShouldntUpdateTheBuiltInCache() {
    $this->itShouldntUpdateAFile('lib/Vuo.framework/Modules/Builtin/libVuoModuleCache.dylib');
    $this->itShouldntUpdateAFile('lib/Vuo.framework/Modules/Builtin/manifest.txt');
    $this->itShouldntUpdateAFile('bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/Builtin/libVuoModuleCache.dylib');
    $this->itShouldntUpdateAFile('bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/Builtin/manifest.txt');
  }

  /**
   * @Then /it should update the ([^ ]*) module in the built-in cache$/
   */
  public function itShouldUpdateAModuleInTheBuiltInCache($module) {
    $this->itShouldUpdateAFile('lib/Vuo.framework/Modules/Builtin/Generated/Modules/' . $this->arch . '/' . $module . '.bc');
    $this->itShouldUpdateAFile('bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/Builtin/Generated/Modules/' . $this->arch . '/' . $module . '.bc');
  }

  /**
   * @Then /it shouldn't update the ([^ ]*) module in the built-in cache$/
   */
  public function itShouldntUpdateAModuleInTheBuiltInCache($module) {
    $this->itShouldntUpdateAFile('lib/Vuo.framework/Modules/Builtin/Generated/Modules/' . $this->arch . '/' . $module . '.bc');
    $this->itShouldntUpdateAFile('bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/Builtin/Generated/Modules/' . $this->arch . '/' . $module . '.bc');
  }

  /**
   * @Then /it shouldn't create ([^ ]*)/
   *
   * Relative to the build folder.
   */
  public function itShouldntCreate($file)
  {
     $buildFile = $this->testDir . '/src/build/' . $file;
     if (is_file($buildFile))
       throw new Exception("I expected the build not to create \"$file\", but it did.");
  }
}
