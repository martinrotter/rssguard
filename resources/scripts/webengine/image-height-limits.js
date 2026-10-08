// For license of this file, see <project-root-folder>/LICENSE.md.

(() => {
  const marker = '@IMAGE_MAXIMUM_HEIGHT_ATTRIBUTE@';
  const ready = 'data-rssguard-image-ready';
  const selector = 'img[' + marker + ']';

  if (window.__rssguardImageLimits) {
    window.__rssguardImageLimits.update();
    return document.querySelector(selector) !== null;
  }

  if (!document.querySelector(selector)) {
    return false;
  }

  const properties = ['width', 'height', 'min-width', 'min-height'];
  const states = new WeakMap();
  const trackedImages = new Set();
  const containerWidths = new WeakMap();
  const observedContainers = new Set();
  let pending = false;
  let updating = false;
  const widthObserver = typeof ResizeObserver === 'undefined' ?
      null :
      new ResizeObserver(entries => {
        let changed = false;
        for (const entry of entries) {
          const width = entry.contentRect.width;
          const previous = containerWidths.get(entry.target);
          containerWidths.set(entry.target, width);
          if (previous === undefined || Math.abs(previous - width) > 0.01) {
            changed = true;
          }
        }
        if (changed) {
          schedule();
        }
      });
  const mutationObserver = new MutationObserver(records => {
    if (records.some(relevantMutation)) {
      schedule();
    }
  });

  function relevantElement(element) {
    return element instanceof Element &&
        (trackedImages.has(element) || element.matches(selector) ||
         element.querySelector(selector) !== null ||
         element.tagName === 'STYLE' || element.tagName === 'LINK');
  }

  function relevantMutation(record) {
    if (relevantElement(record.target)) {
      return true;
    }
    if (record.type === 'childList') {
      for (const node of [...record.addedNodes, ...record.removedNodes]) {
        if (relevantElement(node)) {
          return true;
        }
      }
    }
    return false;
  }

  function observeContainers(images) {
    if (!widthObserver) {
      return;
    }
    const containers = new Set();
    for (const image of images) {
      for (let parent = image.parentElement; parent;
           parent = parent.parentElement) {
        const display = getComputedStyle(parent).display;
        if (display !== 'inline' && display !== 'contents') {
          containers.add(parent);
        }
      }
    }
    for (const container of observedContainers) {
      if (!containers.has(container)) {
        widthObserver.unobserve(container);
        containerWidths.delete(container);
        observedContainers.delete(container);
      }
    }
    for (const container of containers) {
      if (!observedContainers.has(container)) {
        observedContainers.add(container);
        widthObserver.observe(container);
      }
    }
  }

  function declaration(image, property) {
    return {
      value: image.style.getPropertyValue(property),
      priority: image.style.getPropertyPriority(property)
    };
  }

  function stateFor(image) {
    let state = states.get(image);
    if (!state) {
      state = {
        original: {},
        applied: {},
        hadStyle: image.hasAttribute('style')
      };
      for (const property of properties) {
        state.original[property] = declaration(image, property);
      }
      states.set(image, state);
    }
    trackedImages.add(image);
    return state;
  }

  function restore(image, state) {
    for (const property of properties) {
      const applied = state.applied[property];
      if (!applied) {
        state.original[property] = declaration(image, property);
        continue;
      }

      const current = declaration(image, property);
      // Keep publisher changes made since our previous update.
      if (current.value !== applied.value ||
          current.priority !== applied.priority) {
        state.original[property] = current;
      }

      const original = state.original[property];
      if (original.value) {
        image.style.setProperty(property, original.value, original.priority);
      } else {
        image.style.removeProperty(property);
      }
    }
    state.applied = {};
    if (!state.hadStyle && image.style.length === 0) {
      image.removeAttribute('style');
    }
  }

  function apply(image, state, width, height) {
    // Only bounded images override publisher sizing. Other CSS, including
    // stricter maximums, display, object-fit, and decoration, stays in place.
    image.style.setProperty('width', width + 'px', 'important');
    image.style.setProperty('height', height + 'px', 'important');
    image.style.setProperty('min-width', '0px', 'important');
    image.style.setProperty('min-height', '0px', 'important');
    for (const property of properties) {
      state.applied[property] = declaration(image, property);
    }
  }

  function sizeOf(image) {
    const style = getComputedStyle(image);
    return {
      width: parseFloat(style.width),
      height: parseFloat(style.height),
      hidden: style.display === 'none'
    };
  }

  function update() {
    if (updating) {
      return;
    }
    updating = true;
    try {
      for (const image of trackedImages) {
        if (!image.isConnected || !image.hasAttribute(marker)) {
          restore(image, states.get(image));
          image.removeAttribute(ready);
          trackedImages.delete(image);
        }
      }
      const images = Array.from(document.querySelectorAll(selector));

      // Restore every image before measuring, so tables and percentage sizes
      // do not inherit the previous pass's clamped layout.
      for (const image of images) {
        restore(image, stateFor(image));
        image.setAttribute(ready, '');
      }

      const measurements =
          images.map(image => ({
                       image,
                       state: stateFor(image),
                       size: sizeOf(image),
                       limit: Number(image.getAttribute(marker))
                     }));

      for (const {image, state, size, limit} of measurements) {
        if (size.hidden || !Number.isFinite(limit) || limit <= 0) {
          continue;
        }
        if (!Number.isFinite(size.width) || !Number.isFinite(size.height) ||
            size.width <= 0 || size.height <= 0) {
          if (!image.complete) {
            image.removeAttribute(ready);
          }
          continue;
        }
        if (size.height > limit) {
          const scale = limit / size.height;
          apply(image, state, size.width * scale, size.height * scale);
        }
      }

      // A publisher maximum that previously lost to its minimum can become
      // binding after the minimum is released. Keep both axes proportional.
      for (const {image, state, size, limit} of measurements) {
        if (!state.applied.width) {
          continue;
        }
        const intendedScale = limit / size.height;
        const width = size.width * intendedScale;
        const height = size.height * intendedScale;
        const actual = sizeOf(image);
        const constrainedScale =
            Math.min(1, actual.width / width, actual.height / height);
        if (Number.isFinite(constrainedScale) && constrainedScale > 0 &&
            constrainedScale < 0.9999) {
          apply(
              image, state, width * constrainedScale,
              height * constrainedScale);
        }
      }
      // Ancestors can change width without a viewport resize. Watching only
      // width ignores the height changes introduced by our own image limits.
      observeContainers(images);
    } finally {
      // Our own temporary restores and bounded overrides change inline CSS.
      // Consume those records so they cannot repeatedly schedule the helper.
      mutationObserver.takeRecords();
      updating = false;
    }
  }

  function schedule() {
    if (pending) {
      return;
    }
    pending = true;
    requestAnimationFrame(() => {
      pending = false;
      update();
    });
  }

  window.__rssguardImageLimits = {update, schedule};
  mutationObserver.observe(document.documentElement, {
    subtree: true,
    childList: true,
    attributes: true,
    attributeFilter: [
      'style', 'class', 'id', 'width', 'height', 'src', 'srcset', 'sizes',
      'hidden', marker
    ]
  });
  window.addEventListener('resize', schedule);
  window.addEventListener('beforeprint', update);
  window.addEventListener('afterprint', schedule);
  // Chromium switches to print CSS after beforeprint. Recalculate
  // synchronously when that media becomes active, before it paints the PDF.
  const printMedia = window.matchMedia('print');
  if (printMedia.addEventListener) {
    printMedia.addEventListener('change', update);
  } else {
    printMedia.addListener(update);
  }
  for (const event of ['load', 'error']) {
    document.addEventListener(event, event => {
      // Unmarked siblings and delayed stylesheets can change image sizing.
      if (event.target instanceof HTMLImageElement ||
          event.target instanceof HTMLLinkElement) {
        schedule();
      }
    }, true);
  }
  if (document.fonts && document.fonts.ready) {
    document.fonts.ready.then(schedule);
  }
  update();
  return true;
})();
