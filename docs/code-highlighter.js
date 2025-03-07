/**
 * Custom element that loads code from external files and highlights it using highlight.js
 */
class CodeFromFile extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
        this._initialized = false;
        this._loading = false;
    }

    static get observedAttributes() {
        return ['src', 'language', 'highlight-lines'];
    }

    connectedCallback() {
        if (!this._initialized) {
            this._initialized = true;
            this.loadCode();
        }
    }

    attributeChangedCallback(name, oldValue, newValue) {
        // Only reload if the attribute has actually changed and we're not currently loading
        if (oldValue !== newValue && this._initialized && !this._loading) {
            this.loadCode();
        }
    }

    async loadCode() {
        // Prevent concurrent or redundant loads
        if (this._loading) return;
        this._loading = true;

        const src = this.getAttribute('src');
        const language = this.getAttribute('language') || 'cpp';
        const highlightLines = this.getAttribute('highlight-lines') || '';

        if (!src) {
            this.renderError('Error: Missing src attribute');
            this._loading = false;
            return;
        }

        try {
            // Initialize shadow DOM only once
            if (!this.shadowRoot.querySelector('style')) {
                this.initializeShadowDOM();
            } else {
                // Clear only the code container if we're reloading
                const container = this.shadowRoot.querySelector('.code-container');
                if (container) container.remove();
            }

            // Fetch the code file
            const response = await fetch(src);

            if (!response.ok) {
                throw new Error(`Failed to load file: ${response.status} ${response.statusText}`);
            }

            // Get the code as text
            const code = await response.text();

            // Render the code only once
            this.renderCode(code, language, highlightLines);

        } catch (error) {
            this.renderError(`Error loading code: ${error.message}`);
        } finally {
            this._loading = false;
        }
    }

    initializeShadowDOM() {
        // Add CSS for highlight.js
        const linkElem = document.createElement('link');
        linkElem.setAttribute('rel', 'stylesheet');
        linkElem.setAttribute('href', 'https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.7.0/styles/github.min.css');
        this.shadowRoot.appendChild(linkElem);

        // Add custom styles
        const styleElem = document.createElement('style');
        styleElem.textContent = `
        :host {
          display: block;
          position: relative;
          margin: 1em 0;
        }
        
        .code-container {
          position: relative;
        }
        
        pre {
          margin: 0;
          padding: 1em;
          overflow: auto;
          border-radius: 4px;
        }
        
        code {
          font-family: 'SFMono-Regular', Consolas, 'Liberation Mono', Menlo, monospace;
          font-size: 14px;
          line-height: 1.5;
        }
        
        .line-highlight {
          display: block;
          position: absolute;
          left: 0;
          right: 0;
          background-color: rgba(255, 255, 0, 0.2);
          pointer-events: none;
        }
        
        .error {
          color: red;
          padding: 1em;
          border: 1px solid #ffcdd2;
          border-radius: 4px;
          background-color: #ffebee;
        }
      `;
        this.shadowRoot.appendChild(styleElem);
    }

    renderCode(code, language, highlightLines) {
        // Create container for code and highlights
        const container = document.createElement('div');
        container.className = 'code-container';
        this.shadowRoot.appendChild(container);

        // Create pre and code elements
        const preElem = document.createElement('pre');
        const codeElem = document.createElement('code');
        codeElem.className = `language-${language}`;
        codeElem.textContent = code; // Use textContent to avoid HTML parsing

        preElem.appendChild(codeElem);
        container.appendChild(preElem);

        // Apply syntax highlighting only once
        if (typeof hljs !== 'undefined') {
            hljs.highlightElement(codeElem);

            // Apply line highlighting if specified
            if (highlightLines) {
                this.applyLineHighlighting(container, preElem, code, highlightLines);
            }
        } else {
            console.warn('highlight.js not found. Code will be displayed without highlighting.');
        }
    }

    applyLineHighlighting(container, preElem, code, highlightLines) {
        // Parse line numbers to highlight
        const lines = [];
        const codeLines = code.split('\n');
        const lineHeight = 1.5; // em units

        highlightLines.split(',').forEach(part => {
            part = part.trim();
            if (part.includes('-')) {
                const [start, end] = part.split('-').map(num => parseInt(num.trim(), 10));
                for (let i = start; i <= end; i++) {
                    lines.push(i);
                }
            } else if (part) {
                lines.push(parseInt(part, 10));
            }
        });

        // Add highlight elements for each line
        lines.forEach(lineNum => {
            if (lineNum > 0 && lineNum <= codeLines.length) {
                const highlight = document.createElement('div');
                highlight.className = 'line-highlight';

                // Position the highlight
                highlight.style.top = `${(lineNum - 1) * lineHeight + 1}em`; // +1em for padding
                highlight.style.height = `${lineHeight}em`;

                // Insert before the pre element so it appears behind the code
                container.insertBefore(highlight, preElem);
            }
        });
    }

    renderError(message) {
        // Clear existing content
        this.shadowRoot.innerHTML = '';

        // Re-initialize styles
        this.initializeShadowDOM();

        // Add error message
        const errorDiv = document.createElement('div');
        errorDiv.className = 'error';
        errorDiv.textContent = message;
        this.shadowRoot.appendChild(errorDiv);
    }
}

// ==========================================================================

/**
 * CodeBlockManager - Manages code blocks with highlight.js
 * 
 * This class allows you to register and manage code blocks with highlighting,
 * where the code can be defined within script tags in the HTML file.
 */
class CodeBlockManager {
    /**
     * Create a new CodeBlockManager instance
     * @param {Object} options - Configuration options
     * @param {string} options.scriptPrefix - Prefix for script tags containing code (default: 'code-block')
     * @param {string} options.defaultLanguage - Default language for syntax highlighting (default: 'cpp')
     * @param {string} options.highlightTheme - CSS theme for highlight.js (default: 'github')
     */
    constructor(options = {}) {
        this.options = {
            scriptPrefix: options.scriptPrefix || 'code-block',
            defaultLanguage: options.defaultLanguage || 'cpp',
            highlightTheme: options.highlightTheme || 'github'
        };

        this.codeBlocks = {};
        this._initialized = false;
        this._idCounter = 0;
    }

    /**
     * Generate a unique ID for a code block
     * @param {string} customId - Optional custom ID to use instead of auto-generating
     * @returns {string} - A unique ID
     * @private
     */
    _generateId(customId = null) {
        if (customId && !this.codeBlocks[customId]) {
            return customId;
        }

        // Generate a unique ID with a prefix
        const id = `code-block-${++this._idCounter}`;

        // Make sure it's unique
        if (this.codeBlocks[id]) {
            return this._generateId();
        }

        return id;
    }

    /**
     * Initialize the manager by scanning the document for code blocks in script tags
     */
    initialize() {
        if (this._initialized) return;

        // Load CSS theme for highlight.js if not already loaded
        this._loadCss();

        // Scan the document for code blocks in script tags
        this._scanDocument();

        this._initialized = true;
    }

    /**
     * Load highlight.js CSS theme
     * @private
     */
    _loadCss() {
        // Check if the CSS is already loaded
        const existingLink = document.querySelector(`link[href*="highlight.js"][href*="${this.options.highlightTheme}"]`);
        if (existingLink) return;

        const link = document.createElement('link');
        link.rel = 'stylesheet';
        link.href = `https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.7.0/styles/${this.options.highlightTheme}.min.css`;
        document.head.appendChild(link);
    }

    /**
     * Scan the document for code blocks in script tags
     * @private
     */
    _scanDocument() {
        // Find all script tags with the code block prefix
        const scriptElements = document.querySelectorAll(`script[type^="${this.options.scriptPrefix}"]`);

        scriptElements.forEach(scriptEl => {
            // Extract the language from the language attribute or type
            let language = scriptEl.getAttribute('language') || this.options.defaultLanguage;
            let customId = scriptEl.getAttribute('id') || null;

            // Also check if language is specified in the type attribute (backward compatibility)
            // Format: code-block/language or code-block/customId/language
            const parts = scriptEl.type.split('/');
            if (parts.length >= 2 && parts[1] && parts[1] !== customId) {
                // If parts[1] exists and isn't equal to customId, it could be either a customId or language
                if (parts.length >= 3) {
                    // If there are 3+ parts, parts[1] is customId and parts[2] is language
                    customId = customId || parts[1];
                    language = parts[2] || language;
                } else {
                    // If there are only 2 parts, parts[1] could be either language or customId
                    // Check if it looks like a language code
                    if (parts[1].match(/^[a-z0-9]+$/i) && !customId) {
                        language = parts[1];
                    } else {
                        customId = customId || parts[1];
                    }
                }
            }

            // Register the code block
            this.registerBlock(scriptEl.textContent, {
                id: customId,
                language: language,
                elementId: scriptEl.getAttribute('target'),
                highlightLines: scriptEl.getAttribute('highlight-lines')
            });
        });
    }

    /**
     * Register a code block with the manager
     * @param {string} code - Source code content
     * @param {Object} options - Additional options
     * @param {string} options.id - Custom ID to use (optional)
     * @param {string} options.language - Programming language for syntax highlighting
     * @param {string} options.elementId - ID of the element to render the code into
     * @param {string} options.highlightLines - Comma-separated list of line numbers to highlight
     * @returns {string} - The ID of the registered block
     */
    registerBlock(code, options = {}) {
        // Set default options
        const language = options.language || this.options.defaultLanguage;

        // Generate or use custom ID
        const id = this._generateId(options.id);

        // Clean up the code by removing leading/trailing whitespace and normalizing indentation
        const cleanedCode = this._cleanCode(code);

        // Store the code block
        this.codeBlocks[id] = {
            code: cleanedCode,
            language: language,
            elementId: options.elementId,
            highlightLines: options.highlightLines
        };

        // If an element ID is provided, render immediately
        if (options.elementId) {
            const element = document.getElementById(options.elementId);
            if (element) {
                this.renderToElement(id, element);
            }
        }

        return id;
    }

    /**
     * Clean up code by removing leading/trailing whitespace and normalizing indentation
     * @param {string} code - Source code
     * @returns {string} - Cleaned code
     * @private
     */
    _cleanCode(code) {
        // Remove leading and trailing whitespace
        let cleanedCode = code.trim();

        // Detect indentation level
        const lines = cleanedCode.split('\n');
        if (lines.length <= 1) return cleanedCode;

        // Find the minimum indentation level (excluding empty lines)
        const indentMatch = /^(\s+)/.exec(lines.find(line => line.trim().length > 0 && /^\s+/.test(line)));
        if (!indentMatch) return cleanedCode;

        const baseIndent = indentMatch[1];
        const baseIndentRegExp = new RegExp(`^${baseIndent}`);

        // Remove the base indentation from all lines
        cleanedCode = lines
            .map(line => line.replace(baseIndentRegExp, ''))
            .join('\n');

        return cleanedCode;
    }

    /**
     * Escape HTML special characters in code
     * @param {string} code - Source code
     * @returns {string} - Escaped code
     * @private
     */
    _escapeHtml(code) {
        return code
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#039;');
    }

    /**
     * Render a code block to an element
     * @param {string} id - ID of the code block to render
     * @param {HTMLElement} element - Element to render the code into
     */
    renderToElement(id, element) {
        const block = this.codeBlocks[id];
        if (!block) {
            console.error(`Code block with ID '${id}' not found`);
            return;
        }

        if (!element) {
            console.error(`Element to render code block '${id}' into is not provided`);
            return;
        }

        // Create container for code
        const container = document.createElement('div');
        container.className = 'code-container';
        container.style.position = 'relative';

        // Create pre and code elements
        const preElem = document.createElement('pre');
        preElem.style.margin = '0';
        preElem.style.padding = '1em';
        preElem.style.overflow = 'auto';
        preElem.style.borderRadius = '4px';

        const codeElem = document.createElement('code');
        codeElem.className = `language-${block.language}`;
        codeElem.textContent = block.code;
        codeElem.style.fontFamily = "'SFMono-Regular', Consolas, 'Liberation Mono', Menlo, monospace";
        codeElem.style.fontSize = '14px';
        codeElem.style.lineHeight = '1.5';

        // Append elements
        preElem.appendChild(codeElem);
        container.appendChild(preElem);

        // Clear the target element and append the container
        element.innerHTML = '';
        element.appendChild(container);

        // Apply syntax highlighting
        if (typeof hljs !== 'undefined') {
            hljs.highlightElement(codeElem);

            // Apply line highlighting if specified
            if (block.highlightLines) {
                this._applyLineHighlighting(container, preElem, block.code, block.highlightLines);
            }
        } else {
            console.warn('highlight.js not found. Code will be displayed without highlighting.');
        }
    }

    /**
     * Apply line highlighting to a code block
     * @param {HTMLElement} container - Container element for the code block
     * @param {HTMLElement} preElem - Pre element containing the code
     * @param {string} code - Source code
     * @param {string} highlightLines - Comma-separated list of line numbers or ranges to highlight
     * @private
     */
    _applyLineHighlighting(container, preElem, code, highlightLines) {
        // Parse line numbers to highlight
        const lines = [];
        const codeLines = code.split('\n');
        const lineHeight = 1.5; // em units

        highlightLines.split(',').forEach(part => {
            part = part.trim();
            if (part.includes('-')) {
                const [start, end] = part.split('-').map(num => parseInt(num.trim(), 10));
                for (let i = start; i <= end; i++) {
                    lines.push(i);
                }
            } else if (part) {
                lines.push(parseInt(part, 10));
            }
        });

        // Add highlight elements for each line
        lines.forEach(lineNum => {
            if (lineNum > 0 && lineNum <= codeLines.length) {
                const highlight = document.createElement('div');
                highlight.className = 'line-highlight';
                highlight.style.display = 'block';
                highlight.style.position = 'absolute';
                highlight.style.left = '0';
                highlight.style.right = '0';
                highlight.style.backgroundColor = 'rgba(255, 255, 0, 0.2)';
                highlight.style.pointerEvents = 'none';

                // Position the highlight
                highlight.style.top = `${(lineNum - 1) * lineHeight + 1}em`; // +1em for padding
                highlight.style.height = `${lineHeight}em`;

                // Insert before the pre element so it appears behind the code
                container.insertBefore(highlight, preElem);
            }
        });
    }

    /**
     * Render all code blocks to their corresponding elements
     */
    renderAll() {
        // Ensure we're initialized
        if (!this._initialized) {
            this.initialize();
        }

        // Render each code block that has a target element
        Object.entries(this.codeBlocks).forEach(([id, block]) => {
            if (block.elementId) {
                const element = document.getElementById(block.elementId);
                if (element) {
                    this.renderToElement(id, element);
                } else {
                    console.warn(`Target element with ID '${block.elementId}' for code block '${id}' not found`);
                }
            }
        });

        // Also render code blocks to elements with code-block attributes
        const codeElements = document.querySelectorAll('[code-block]');
        codeElements.forEach(element => {
            const id = element.getAttribute('code-block');
            if (this.codeBlocks[id]) {
                this.renderToElement(id, element);
            } else {
                console.warn(`Code block '${id}' not found for element with code-block attribute`);
            }
        });
    }

    /**
     * Get a code block by ID
     * @param {string} id - ID of the code block
     * @returns {Object|null} - The code block or null if not found
     */
    getBlock(id) {
        return this.codeBlocks[id] || null;
    }

    /**
     * Get the source code for a block by ID
     * @param {string} id - ID of the code block
     * @returns {string|null} - The source code or null if not found
     */
    getCode(id) {
        const block = this.getBlock(id);
        return block ? block.code : null;
    }

    /**
     * Find a code block by target element ID
     * @param {string} targetId - Target element ID
     * @returns {string|null} - The ID of the code block or null if not found
     */
    findBlockByTargetId(targetId) {
        for (const [id, block] of Object.entries(this.codeBlocks)) {
            if (block.elementId === targetId) {
                return id;
            }
        }
        return null;
    }
}

// ==========================================================================

// Register the custom element
customElements.define('code-from-file', CodeFromFile);

// Create and export a singleton instance
const codeBlockManager = new CodeBlockManager();

// Auto-initialize when the DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    codeBlockManager.renderAll();
});
