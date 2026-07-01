import { Sandbox } from '@vercel/sandbox';

async function main() {
  const binUrl = process.env.COMPILER_BIN_URL;
  if (!binUrl) throw new Error('COMPILER_BIN_URL is not set');

  const sandbox = await Sandbox.create({
    token: process.env.VERCEL_TOKEN,
    teamId: process.env.VERCEL_TEAM_ID,
    projectId: process.env.VERCEL_PROJECT_ID,
    runtime: 'node24',
    timeout: 300_000,
  });

  try {
    const setup = await sandbox.runCommand('sh', [
      '-c',
      `sudo dnf install -y gcc && curl -fsSL "${binUrl}" -o c-- && chmod +x c--`,
    ]);
    if (setup.exitCode !== 0) {
      throw new Error(`Sandbox setup failed: ${await setup.stderr()}`);
    }

    const snapshot = await sandbox.snapshot();
    console.log(snapshot.snapshotId);
  } catch (err) {
    await sandbox.stop().catch(() => {});
    throw err;
  }
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
