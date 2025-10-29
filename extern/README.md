# External Dependencies

JUCE is consumed as a git submodule at `extern/JUCE`. Add it via:

```bash
git submodule add https://github.com/juce-framework/JUCE.git extern/JUCE
```

Then ensure it is updated for all developers:

```bash
git submodule update --init --recursive
```

The JUCE sources are not committed in this repository snapshot.
